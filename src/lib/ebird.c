#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define _GNU_SOURCE 500
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <oauth.h>

#include <Eina.h>
#include <Eet.h>
#include <Ecore.h>
#include <Ecore_Con.h>
#include <Ecore_File.h>

#include "Ebird.h"
#include "ebird_private.h"

static int _ebird_main_count = 0;

/* log domain variable */
int _ebird_log_dom_global = -1;

static Eina_Bool _url_data_cb(void *data,
                              int   type,
                              void *event_info);
static Eina_Bool _url_complete_cb(void *data,
                                  int   type,
                                  void *event_info);
static void ebird_timeline_get(const char *url,
                               Async_Data *d);

static const char *_ebird_config_dir_get(const char *suffix);

/*
 * Name   : ebird_init
 * Params : none
 * Aim    : initialize the library load starting stuff
 *
 */
EAPI int
ebird_init()
{
   const char *dir;
   //Eina_Prefix *pfx = NULL;

   if (EINA_LIKELY(_ebird_main_count > 0))
     return ++_ebird_main_count;

   if (!eina_init())
     return EINA_FALSE;
   if (!ecore_init())
     goto shutdown_eina;
   if (!ecore_con_init())
     goto shutdown_ecore;
   if (!ecore_con_url_init())
     goto shutdown_ecore_con;
   if (!ecore_file_init())
     goto shutdown_ecore_file;
   if (!eet_init())
     goto shutdown_ecore_con_url;

   _ebird_log_dom_global = eina_log_domain_register("ebird", EBIRD_DEFAULT_LOG_COLOR);

   if (_ebird_log_dom_global < 0)
     {
        EINA_LOG_ERR("Ebird Can not create a general log domain.");
        goto shutdown_ecore_con_url;
     }
   
   EBIRD_EVENT_AVATAR_DOWNLOAD = ecore_event_type_new();
   EBIRD_EVENT_PIN_NEED = ecore_event_type_new();

   dir = _ebird_config_dir_get(NULL);
   if (!ecore_file_is_dir(dir))
      ecore_file_mkdir(dir);

   dir = _ebird_config_dir_get(EBIRD_IMAGES_CACHE);
   if (!ecore_file_is_dir(dir))
      ecore_file_mkdir(dir);
   
   /*
   pfx = eina_prefix_new(NULL, ebird_init, "EBIRD", "Ebird", NULL,
                         PACKAGE_BIN_DIR,
                         PACKAGE_LIB_DIR,
                         PACKAGE_DATA_DIR,
                         "/tmp");
   if (!pfx) printf("ERROR: Critical error in finding prefix\n");
   printf("install prefix is: %s\n", eina_prefix_get(pfx));
   printf("binaries are in: %s\n", eina_prefix_bin_get(pfx));
   printf("libraries are in: %s\n", eina_prefix_lib_get(pfx));
   printf("data files are in: %s\n", eina_prefix_data_get(pfx));
   eina_prefix_free(pfx);
   *
   */

    printf("data files are in: %s\n", PACKAGE_DATA_DIR);

   DBG("Ebird Init done");

   _ebird_main_count = 1;
   return 1;

shutdown_ecore_file:
   ecore_file_shutdown();
shutdown_ecore_con_url:
   ecore_con_url_shutdown();
shutdown_ecore_con:
   ecore_con_shutdown();
shutdown_ecore:
   ecore_shutdown();
shutdown_eina:
   eina_shutdown();

   return 0;
}

/*
 * Name   : ebird_shutdown
 * Params : none
 * Aim    : shutdown things
 *
 */
EAPI int
ebird_shutdown()
{
   _ebird_main_count--;
   if (EINA_UNLIKELY(_ebird_main_count == 0))
     {
        eet_shutdown();
        ecore_con_url_shutdown();
        ecore_con_shutdown();
        ecore_file_shutdown();
        ecore_shutdown();
        eina_shutdown();
     }
   return _ebird_main_count;
}

/*
 * Name   : ebird_add
 * Params : none
 * Aim    : Create and initialize and Ebird_Object
 *          This object have to be destroyed by ebird_del.
 * Return : Ebird_Object *
 */
EAPI Ebird_Object *
ebird_add(void)
{
   Ebird_Object *eobj;

   eobj = calloc(1, sizeof(Ebird_Object));
   eobj->request_token = calloc(1, sizeof(OauthToken));
   eobj->account = calloc(1, sizeof(EbirdAccount));
   ebird_id_load(eobj->request_token);

   return eobj;
}

/*
 * Name   : ebird_del
 * Params : Ebird_Object *
 * Aim    : Free an Ebird_Object created by ebird_add
 * Return : void
 */
EAPI void
ebird_del(Ebird_Object *eobj)
{
   if (eobj->request_token)
     free(eobj->request_token);

   if (eobj->account)
     free(eobj->account);
}

/*
 *  Name   : ebird_oauth_sign_url
 *  Params :
 *      url             -> The API method URL
 *      consumer_key    -> The application consumer key
 *      consumer_secret -> The application consumer secret key
 *      token_key       -> The current session token key
 *      token_secret    -> The current session token secret key
 *      http_data       -> The http action POST or GET
 *  Aim    : Provide an oauth signed URL to call an API method
 *           the return have to be free.
 *  Return : char *
 */
static char *
ebird_oauth_sign_url(const char *url,
                     const char *consumer_key,
                     const char *consumer_secret,
                     const char *token_key,
                     const char *token_secret,
                     const char *http_method)
{
   char *out_url;

   out_url = oauth_sign_url2(url,
                             NULL,
                             OA_HMAC,
                             http_method,
                             consumer_key,
                             consumer_secret,
                             token_key,
                             token_secret);

   DBG("[SIGNED-URL] : [%s]", out_url);

   return out_url;
}

/*
 *  Name   : _url_data_cb
 *  Params :
 *      data       -> Data provided to the callback by ecore_event
 *                    In this case it contains an Async_Data
 *      type       -> ...
 *      event_info -> Data provided by http connection
 *  Aim    : Action when some data is provided after an ecore_con_url_get
 *  Return : Eina_Bool
 */
static Eina_Bool
_url_data_cb(void *data,
             int   type,
             void *event_info)
{
   Async_Data *d = data;
   Ecore_Con_Event_Url_Data *url_data = event_info;

   if (d->url != url_data->url_con)
     return EINA_TRUE;

   if (!d->http_data)
     d->http_data = eina_strbuf_new();

   eina_strbuf_append_length(d->http_data,
                             url_data->data,
                             url_data->size);

   return EINA_TRUE;
}

EAPI Eina_Bool
ebird_account_save(Ebird_Object *obj)
{
   Eet_File *file;
   int size;

   
   DBG("Opening %s", _ebird_config_dir_get(EBIRD_ACCOUNT_FILE));
   file = eet_open(_ebird_config_dir_get(EBIRD_ACCOUNT_FILE), EET_FILE_MODE_WRITE);
   DBG("Saving username");
   eet_write(file, "username", obj->account->username,
             strlen(obj->account->username) + 1, 0);
   DBG("Saving Password");
   eet_write(file, "passwd", obj->account->passwd,
             strlen(obj->account->passwd) + 1, 1);
   DBG("Saving Access Token Key");
   eet_write(file, "access_token_key", obj->account->access_token_key,
             strlen(obj->account->access_token_key) + 1, 0);
   DBG("Saving Access Token Secret");
   eet_write(file, "access_token_secret", obj->account->access_token_secret,
             strlen(obj->account->access_token_secret) + 1, 0);
   DBG("Saving User ID");
   eet_write(file, "userid", obj->account->userid,
             strlen(obj->account->userid) + 1, 0);
   DBG("Saving Avatar image");
   if (obj->account->avatar)
     eet_write(file, "avatar", obj->account->avatar,
               strlen(obj->account->avatar) + 1, 0);

   DBG("Closing %s", _ebird_config_dir_get(EBIRD_ACCOUNT_FILE));
   eet_close(file);

   return EINA_TRUE;
}

EAPI Eina_Bool
ebird_account_load(Ebird_Object *obj)
{
   Eet_File *file;
   int size;

   if (!obj)
      return EINA_FALSE;

   if (!ecore_file_exists(_ebird_config_dir_get(EBIRD_ACCOUNT_FILE)))
      return EINA_FALSE;

   file = eet_open(_ebird_config_dir_get(EBIRD_ACCOUNT_FILE), EET_FILE_MODE_READ);
   obj->account->username = eina_stringshare_add(eet_read(file, "username", &size));
   obj->account->passwd = eina_stringshare_add(eet_read(file, "passwd", &size));
   obj->account->access_token_key = eina_stringshare_add(eet_read(file, "access_token_key", &size));
   obj->account->access_token_secret = eina_stringshare_add(eet_read(file, "access_token_secret", &size));
   obj->account->userid = eina_stringshare_add(eet_read(file, "userid", &size));

   eet_close(file);

   return EINA_TRUE;
}

EAPI int
ebird_id_load(OauthToken *request_token)
{
   Eet_File *file;
   int size;

   file = eet_open(PACKAGE_DATA_DIR"/"EBIRD_ID_FILE, EET_FILE_MODE_READ);
   request_token->consumer_key = strdup(eet_read(file, "key", &size));
   request_token->consumer_secret = strdup(eet_read(file, "secret", &size));
   eet_close(file);
}

/*
 * name: ebird_error_code_get
 * @param : web script retruned by ebird_http_get()
 * @return : Error code
 */

int
ebird_error_code_get(char *string)
{
   int compare_res;

   compare_res = strcmp(string, "Failed to validate oauth signature and token");
   if (compare_res == 0)
     return 500;
   else
     return 0;
}

int
ebird_token_authenticity_get(Async_Data *data)
{
   char *key,
   *end;
   const char *web_script;

   if (data->http_data)
      web_script = eina_strbuf_string_get(data->http_data);
   else
      return 1;

   //printf("DEBUG webscript=%s\n", web_script);
   key = strstr(web_script, "twttr.form_authenticity_token");
   if (!key)
     return -1;

   key = strchr(key, '\'');
   if (!key)
     return -1;
   key++;
   end = strchr(key, '\'');
   if (!end)
     return -1;

   *end = '\0';
   data->eobj->request_token->authenticity_token = eina_stringshare_add(key);
   *end = '\'';

   return 0;
}

Eina_Bool
ebird_file_exists(char *filename)
{
   return ecore_file_exists(filename);
}

char *
ebird_avatar_filename_get(char *url)
{
   char **array;
   char *filename;
   unsigned int elms;

   array = eina_str_split_full(url, "/", 0, &elms);

   return array[elms - 1];
}

static Eina_Bool
_wget_cb(void *data,
         int   type,
         void *event_info)
{
   Async_Data *d = data;
   Ecore_Event_Handler *h;
   
   ecore_event_add(EBIRD_EVENT_AVATAR_DOWNLOAD,d,NULL,NULL);
   
   DBG("ebird_wget data complete callback");

   EINA_LIST_FREE(d->handlers, h)
     ecore_event_handler_del(h);

   d->handlers = NULL;
}

char *
ebird_wget(char *url, const char *prefix)
{
   Async_Data *d;
   Ecore_Event_Handler *h;
   int fd;
   char *file;
   int res;
   char filename[EBIRD_URL_MAX];

   DBG("DEBUG : ebird_wget_start !");

   d = calloc(1, sizeof(Async_Data));

   file = ebird_avatar_filename_get(url);

   snprintf(filename, sizeof(filename), "%s/%s-%s", _ebird_config_dir_get(EBIRD_IMAGES_CACHE), prefix, file);

   DBG("\nFILENAME [%s]]", filename);

   if ( !ebird_file_exists(filename) || ecore_file_size(filename) == 0)
     {
        DBG("FILE [%s] have to be downloaded from [%s]", filename, url);

        d->url = ecore_con_url_new(url);

        h = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                                    _wget_cb, d);
        d->handlers = eina_list_append(d->handlers, h);

        d->fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        ecore_con_url_fd_set(d->url, d->fd);

        ecore_con_url_get(d->url);
     }
   DBG("RETURN : %s",filename);
   return strdup(filename);
}

static Eina_Bool
_parse_user(void                *data,
            Eina_Simple_XML_Type type,
            const char          *content,
            unsigned             offset,
            unsigned             length)
{
   static EbirdAccount *cur = NULL;
   static UserState s = USER_NONE;

   printf("\n\nPARSE USER\n");
   //printf("%s\n",content);

   data = (EbirdAccount *)data;

   //if (type == EINA_SIMPLE_XML_OPEN && !strncmp("user", content, 4))
   if (type == EINA_SIMPLE_XML_OPEN)
     {
        if (!strncmp("screen_name", content, 11))
          s = SCREEN_NAME;
        else if (!strncmp("id", content, 2))
          s = USER_ID;
        else if (!strncmp("profile_image_url_https", content, 23))
          s = AVATAR;
     }
   else if (cur && type == EINA_SIMPLE_XML_DATA)
     {
        char *ptr = strndup(content, length);
        switch(s)
          {
           case SCREEN_NAME:
             if ( !cur->username)
               cur->username = ptr;
             break;

           case AVATAR:
             DBG("===> [DEBUG][%s]", ptr);
             cur->avatar = ebird_wget(ptr,cur->username);
             break;

           case USER_ID:
             cur->userid = ptr;
             break;
          }
     }
   else if (cur && type == EINA_SIMPLE_XML_CLOSE &&
            !strncmp("user", content, 4))
     data = cur;

   return EINA_TRUE;
}

static Eina_Bool
_parse_timeline(void                *_data,
                 Eina_Simple_XML_Type type,
                 const char          *content,
                 unsigned             offset,
                 unsigned             length)
{
   static EbirdStatus *current = NULL;
   static State s = NONE;

   void **data = (void **)_data;

   if (type == EINA_SIMPLE_XML_OPEN && !strncmp("status", content, length))
     {
        current = calloc(1, sizeof(EbirdStatus));
        current->created_at = NULL;
        current->user = calloc(1, sizeof(EbirdAccount));
        current->retweeted_status = calloc(1, sizeof(EbirdStatus));
        current->retweeted_status->user = calloc(1, sizeof(EbirdAccount));
     }
   else if (current && type == EINA_SIMPLE_XML_OPEN)
     {
        if (!strncmp("retweeted_status", content, 16))
          {
             s = RETWEETED;
             current->retweeted = EINA_TRUE;
          }
        else if (!strncmp("created_at", content, 10))
          {
             if (!current->created_at)
               s = CREATEDAT;
             else
               s = NONE;
          }
        else if (!strncmp("text", content, 4))
          {
             s = TEXT;
          }
        else if (!strncmp("id", content, 2))
          {
             s = ID;
          }
        else if (!strncmp("user", content, 4))
          {
             s = USER;
          }
        else if (!strncmp("screen_name", content, 11))
          {
             s = USERNAME;
          }
        else if (!strncmp("name", content, 4))
          {
             s = REALNAME;
          }
        else if (!strncmp("profile_image_url_https", content, 23))
          {
             s = IMAGE;
          }
        else
          {
             s = NONE;
          }
     }
   else if (current && type == EINA_SIMPLE_XML_DATA)
     {
        char *ptr = strndup(content, length);
        if (!current->retweeted)
          {
             switch(s)
               {
                case CREATEDAT:
                  current->created_at = ptr;
                  break;

                case TEXT:
                  current->text = ptr;
                  break;

                case ID:
                  if (! current->id)
                     current->id = ptr;
                  else if ( current->id && current->user)
                     current->user->userid = ptr;
                  break;

                case IMAGE:
                  current->user->avatar = ebird_wget(ptr,current->user->username);
                  break;

                case REALNAME:
                  current->user->realname = ptr;
                  break;

                case USERNAME:
                  current->user->username = ptr;
                  break;
               }
          }
        else
          {
             switch(s)
               {
                case CREATEDAT:
                  current->created_at = ptr;
                  break;

                case TEXT:
                  current->retweeted_status->text = ptr;
                  break;

                case ID:
                  current->retweeted_status->id = ptr;
                  break;

                case IMAGE:
                  current->retweeted_status->user->avatar = ebird_wget(ptr, current->retweeted_status->user->username);
                  break;

                case USERNAME:
                  current->retweeted_status->user->username = ptr;
                  break;
               }
          }
     }
   else if (current && type == EINA_SIMPLE_XML_CLOSE && !strncmp("retweeted_status", content, 16))
     {
        current->retweeted = EINA_FALSE;
     }
   else if (current && type == EINA_SIMPLE_XML_CLOSE && !strncmp("status", content, 6))
     {
        if ( ! current->retweeted)
        {
           if (current->text)
           {
              if (!strncmp("RT ", current->text, 3))
                {
                   current->retweeted = EINA_TRUE;
                }
             }
           DBG("eina_list_append");
           *data = eina_list_append(*data, current);
           current = NULL;
        }
     }
     return EINA_TRUE;
}

/* TO REMOVE
static Eina_Bool
_parse_timeline(void                *_data,
                Eina_Simple_XML_Type type,
                const char          *content,
                unsigned             offset,
                unsigned             length)
{
   static EbirdStatus *cur = NULL;
   static State s = NONE;
   static State rt_s = NONE;
   static UserState us = USER_NONE;
   static UserState rt_us = USER_NONE;

   void **data = (void **)_data;

   if (type == EINA_SIMPLE_XML_OPEN && !strncmp("status", content, length))
     {
        cur = calloc(1, sizeof(EbirdStatus));
        cur->user = calloc(1, sizeof(EbirdAccount));
     }
   else if (cur && type == EINA_SIMPLE_XML_OPEN)
     {
        us = USER_NONE;

        if (!strncmp("retweeted_status", content, 16))
          {
             s = RETWEETED;
             cur->retweeted_status = calloc(1, sizeof(EbirdStatus));
             cur->retweeted_status->user = calloc(1, sizeof(EbirdAccount));
             cur->retweeted = EINA_TRUE;
          }
        else if (!strncmp("created_at", content, 10))
          s = CREATEDAT;
        else if (!strncmp("text", content, 4))
          s = TEXT;
        else if (!strncmp("id", content, 2))
          s = ID;
        else if (!strncmp("user", content, 4))
          s = USER;
        else if (!strncmp("screen_name", content, 11))
          {
             us = SCREEN_NAME;
             s = USER;
          }
        else if (!strncmp("profile_image_url_https", content, 23))
          {
             us = AVATAR;
             s = USER;
          }
        else
          s = NONE;
     }
   else if (cur && type == EINA_SIMPLE_XML_DATA)
     {
        char *ptr = strndup(content, length);
        if ( !cur->retweeted)
          {
             switch(s)
               {
                case CREATEDAT:
                  cur->created_at = ptr;
                  break;

                case TEXT:
                  cur->text = ptr;
                  break;

                case ID:
                  cur->id = ptr;
                  break;

                case USER:
                  printf("user\n");
                  break;
               }

             switch(us)
               {
                case SCREEN_NAME:
                  cur->user->username = ptr;
                  break;

                case AVATAR:
                  cur->user->avatar = ebird_wget(ptr);
                  break;

                case USER_NONE:
                  break;
               }
          }
        else
          {
             switch(s)
               {
                case CREATEDAT:
                  cur->retweeted_status->created_at = ptr;
                  break;

                case TEXT:
                  cur->retweeted_status->text = ptr;
                  break;

                case ID:
                  cur->retweeted_status->id = ptr;
                  break;

                case RETWEETED:
                  cur->retweeted_status->retweeted = EINA_TRUE;
                  break;

                case USER:
                  break;
               }

             switch(us)
               {
                case SCREEN_NAME:
                  cur->retweeted_status->user->username = ptr;
                  break;

                case AVATAR:
                  cur->retweeted_status->user->avatar = ebird_wget(ptr);
                  cur->retweeted = EINA_FALSE;

                case USER_NONE:
                  break;
               }
          }
     }
   else if (cur && type == EINA_SIMPLE_XML_CLOSE && !strncmp("user", content, 4))
     s = NONE;
   else if (cur && type == EINA_SIMPLE_XML_CLOSE && !strncmp("retweeted_status", content, 16))
     {
        cur->retweeted = EINA_FALSE;
     }
   else if (cur && type == EINA_SIMPLE_XML_CLOSE && !strncmp("status", content, 6) && !cur->retweeted)
     {
        if (!strncmp("RT ", cur->text, 3))
          cur->retweeted = EINA_TRUE;

        DBG("eina_list_append");
        *data = eina_list_append(*data, cur);
        cur = NULL;
     }
   else if (cur && type == EINA_SIMPLE_XML_CLOSE && !strncmp("retweeted_status", content, 16))
     {
        if (cur->retweeted)
          cur->retweeted = EINA_FALSE;
     }
   return EINA_TRUE;
}
*/

EAPI void
ebird_timeline_free(Eina_List *timeline)
{
   EbirdStatus *st;

   EINA_LIST_FREE(timeline, st)
     {
        if (st->retweeted && st->retweeted_status)
          {
             free(st->retweeted_status->user);
             free(st->retweeted_status);
             free(st->user);
             free(st);
          }
        else
          {
             free(st->user);
             free(st);
          }
     }
}

static Eina_Bool
_ebird_timeline_get_cb(void *data,
                       int   type,
                       void *event_info)
{
   const char *xml;
   Eina_List *timeline = NULL;
   Ecore_Event_Handler *h;
   Ecore_Con_Event_Url_Complete *url = event_info;
   Async_Data *d = data;
   Ebird_Object *eobj = d->eobj;
   EbirdStatus *lastmsg = NULL;

   if (d->url != url->url_con)
     return EINA_TRUE;

   if (d->http_data)
     xml = eina_strbuf_string_get(d->http_data);
   else
     xml = eina_stringshare_add("No timeline");

   DBG("%s", xml);
   eina_simple_xml_parse(xml, strlen(xml), EINA_TRUE, _parse_timeline, &timeline);

   if (timeline)//lastmsg = eina_list_last(timeline);
   {
      lastmsg = eina_list_nth(timeline, 0);
      DBG("NTH ID [%s]\n", lastmsg->id);
      eobj->newer_msg_id = lastmsg->id;
      DBG("newer_msg_id = %s\n", eobj->newer_msg_id);
   }

   EINA_LIST_FREE(d->handlers, h)
   {
     ecore_event_handler_del(h);
   }
   d->handlers = NULL;


   if (d->cb)
   {
     d->cb(eobj, d->data, timeline);
   }
}

static void
ebird_timeline_get(const char *url,
                   Async_Data *d)
{
   Ecore_Event_Handler *h;
   Ebird_Object *eobj = d->eobj;
   char *sig_url;
   char full_url[EBIRD_URL_MAX];

   if (d->eobj->newer_msg_id)
     {
        snprintf(full_url, sizeof(full_url),
                 "%s&since_id=%s",
                 url,
                 d->eobj->newer_msg_id);
        sig_url = ebird_oauth_sign_url(full_url,
                                       eobj->request_token->consumer_key,
                                       eobj->request_token->consumer_secret,
                                       eobj->account->access_token_key,
                                       eobj->account->access_token_secret, NULL);
        DBG("SIGNED URL IS [%s]",sig_url);
     }
   else
     {
        sig_url = ebird_oauth_sign_url(url,
                                       eobj->request_token->consumer_key,
                                       eobj->request_token->consumer_secret,
                                       eobj->account->access_token_key,
                                       eobj->account->access_token_secret, NULL);
     }

   d->url = ecore_con_url_new(sig_url);
   DBG("TIMELINE_GET_URL [%s]\n", sig_url);

   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                               _url_data_cb, d);
   DBG("eina_list_append");
   d->handlers = eina_list_append(d->handlers, h);
   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                               _ebird_timeline_get_cb, d);
   DBG("eina_list_append");
   d->handlers = eina_list_append(d->handlers, h);

   ecore_con_url_get(d->url);
}

EAPI void
ebird_timeline_home_get(Ebird_Object    *eobj,
                        Ebird_Session_Cb cb,
                        void            *data)
{
   Async_Data *d;

   if (!eobj)
     return;

   d = calloc(1, sizeof(Async_Data));
   d->eobj = eobj;

   d->cb = cb;
   d->data = data;

   ebird_timeline_get(EBIRD_HOME_TIMELINE_URL, d);
}

EAPI void
ebird_timeline_public_get(Ebird_Object    *eobj,
                          Ebird_Session_Cb cb,
                          void            *data)
{
   Async_Data *d;

   if (!eobj)
     return;

   d = calloc(1, sizeof(Async_Data));
   d->eobj = eobj;

   d->cb = cb;
   d->data = data;

   ebird_timeline_get(EBIRD_PUBLIC_TIMELINE_URL, d);
}

EAPI void
ebird_timeline_user_get(Ebird_Object    *eobj,
                        Ebird_Session_Cb cb,
                        void            *data)
{
   Async_Data *d;

   if (!eobj)
     return;

   d = calloc(1, sizeof(Async_Data));
   d->eobj = eobj;

   d->cb = cb;
   d->data = data;

   ebird_timeline_get(EBIRD_USER_TIMELINE_URL, d);
}

EAPI void
ebird_timeline_mentions_get(Ebird_Object    *eobj,
                            Ebird_Session_Cb cb,
                            void            *data)
{
   Async_Data *d;

   if (!eobj)
     return;

   d = calloc(1, sizeof(Async_Data));
   d->eobj = eobj;

   d->cb = cb;
   d->data = data;

   ebird_timeline_get(EBIRD_USER_MENTIONS_URL, d);
}

static Eina_Bool
_ebird_status_update_cb(void *data,
                        int   type,
                        void *event_info)
{
   Ecore_Event_Handler *h;
   Async_Data *d = data;
   Ebird_Object *eobj = d->eobj;
   const char *xml;
   Eina_List *timeline = NULL;
   Ecore_Con_Event_Url_Complete *url = event_info;

   if (d->url != url->url_con)
     return EINA_TRUE;

   if (d->http_data)
      xml = eina_strbuf_string_get(d->http_data);
   else
      return EINA_FALSE;

   DBG("%s\n", xml);
   printf("%s\n",xml);
   eina_simple_xml_parse(xml, strlen(xml), EINA_TRUE, _parse_timeline, &timeline);

   EINA_LIST_FREE(d->handlers, h)
     ecore_event_handler_del(h);
   d->handlers = NULL;

   if (d->cb)
     d->cb(eobj, d->data, timeline);

   return EINA_TRUE;
}

EAPI void
ebird_status_update(char            *message,
                    Ebird_Object    *obj,
                    Ebird_Session_Cb cb,
                    void            *data)
{
   Async_Data *d;
   Ecore_Event_Handler *h;

   Ecore_Con_Url *ec_url;

   char *sig_url;
   char full_url[EBIRD_URL_MAX];

   d = calloc(1, sizeof(Async_Data));
   d->eobj = obj;

   d->cb = cb;
   d->data = data;

   DBG("Update Status Start Here !");

   snprintf(full_url, sizeof(full_url),
            "%s&status=%s&include_entities=true",
            EBIRD_STATUS_URL,
            message);

   sig_url = ebird_oauth_sign_url(full_url,
                                  obj->request_token->consumer_key,
                                  obj->request_token->consumer_secret,
                                  obj->account->access_token_key,
                                  obj->account->access_token_secret, "POST");


   data = eina_strbuf_new();

   d->url = ecore_con_url_new(sig_url);
   DBG("STATUS_UPDATE_URL [%s]\n", full_url);
   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                               _url_data_cb, d);
   DBG("eina_list_append");
   d->handlers = eina_list_append(d->handlers, h);
   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                               _ebird_status_update_cb, d);
   DBG("eina_list_append");
   d->handlers = eina_list_append(d->handlers, h);

   ecore_con_url_get(d->url);
}

EAPI Eina_Bool
ebird_user_show(EbirdAccount *acc)
{
   if (acc)
     {
        if (acc->username)
          INF("USERNAME : [%s]", acc->username);
        if (acc->userid)
          INF("USERID : [%s]", acc->userid);
        if (acc->avatar)
          INF("AVATAR : [%s]", acc->avatar);
     }
   else
     {
        WRN("Nothing to show for this user");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static Eina_Bool
_ebird_access_token_get_cb(void *data,
                           int   type,
                           void *event_info)
{
   Async_Data *d = data;
   Ebird_Object *obj = d->eobj;
   const char *acc_token;
   char **access_token_prm;
   int res;
   Ecore_Con_Event_Url_Complete *url = event_info;

   if (d->url != url->url_con)
     return EINA_TRUE;

   access_token_prm = (char **)malloc(5 * sizeof(char *));

   if (d->http_data)
   {
      acc_token = eina_strbuf_string_get(d->http_data);
      DBG("DATA : %s", acc_token);
   }
   else
      return EINA_FALSE;

   if (acc_token)
     {
        DBG("DEBUG[ebird_access_token_get][RESULT]{%s}", acc_token);
        //request_token->access_token = strdup(acc_token);

        res = oauth_split_url_parameters(acc_token, &access_token_prm);
        if (res == 4)
          {
             DBG("SPLIT OK");
             obj->account->access_token_key = strdup(&(access_token_prm[0][12]));
             DBG("ACCESS_TOKEN_KEY[%s]\n", obj->account->access_token_key);
             obj->account->access_token_secret = strdup(&(access_token_prm[1][19]));
             DBG("ACCESS_TOKEN_SECRET[%s]\n", obj->account->access_token_secret);
             obj->account->userid = strdup(&(access_token_prm[2][8]));
             obj->account->username = strdup(&(access_token_prm[3][12]));
             obj->account->passwd = strdup("nill");
             DBG("SAVING CONFIGURATION");
             ebird_account_save(obj);
          }
        else
          {
             ERR("Error on access_token split");
             ERR("%s", acc_token);
             ERR("[%i]", res);

             return EINA_FALSE;
          }
     }
   free(access_token_prm);
   d->cb(obj, d->data, NULL);
}

void
ebird_access_token_get(Async_Data *d)
{
   Ecore_Event_Handler *h;
   Ebird_Object *obj = d->eobj;
   char *acc_url;
   char buf[EBIRD_URL_MAX];

   DBG("Start of access token get");

   EINA_LIST_FREE(d->handlers, h)
     ecore_event_handler_del(h);

   d->handlers = NULL;

   acc_url = ebird_oauth_sign_url(EBIRD_ACCESS_TOKEN_URL,
                                  obj->request_token->consumer_key,
                                  obj->request_token->consumer_secret,
                                  obj->request_token->key,
                                  obj->request_token->secret, NULL);

   snprintf(buf, sizeof(buf), "%s&oauth_verifier=%s",
            acc_url,
            d->eobj->request_token->authorisation_pin);

   d->http_data = eina_strbuf_new();
   d->url = ecore_con_url_new(buf);
   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                               _ebird_access_token_get_cb, d);
   DBG("eina_list_append");
   d->handlers = eina_list_append(d->handlers, h);
   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                               _url_data_cb, d);
   DBG("eina_list_append");
   d->handlers = eina_list_append(d->handlers, h);

   DBG("[URL][%s]", buf);
   ecore_con_url_get(d->url);
   free(acc_url);
}

EAPI Eina_Bool
ebird_authorisation_pin_set(Ebird_Object *obj,
                            char         *pin)
{
   obj->request_token->authorisation_pin = strdup(pin);
   return EINA_TRUE;
}

Eina_Bool
ebird_pin_set(Ebird_Object *obj,char *pin)
{
   obj->request_token->authorisation_pin = strdup(pin);
}

Eina_Bool
ebird_read_pin_from_stdin(Ebird_Object *obj)
{
   char buffer[EBIRD_PIN_SIZE];
   char url[EBIRD_URL_MAX];

   snprintf(url, sizeof(url),
            EBIRD_DIRECT_TOKEN_URL "?authenticity_token=%s&oauth_token=%s",
            obj->request_token->authenticity_token,
            obj->request_token->key);

   obj->request_token->authorisation_url = eina_stringshare_add(url);

   printf("Open this url in a web browser to authorize ebird \
to access to your account.\n%s\n",
          obj->request_token->authorisation_url);

   puts("Please enter PIN here :");
   fgets(buffer, sizeof(buffer), stdin);
   buffer[strlen(buffer) - 1] = '\0';

   obj->request_token->authorisation_pin = strdup(buffer);

   return EINA_TRUE;
}

Eina_Bool
ebird_authorisation_url_send(Ebird_Object *obj)
{
   char buffer[EBIRD_PIN_SIZE];
   char url[EBIRD_URL_MAX];

   snprintf(url, sizeof(url),
            EBIRD_DIRECT_TOKEN_URL "?authenticity_token=%s&oauth_token=%s",
            obj->request_token->authenticity_token,
            obj->request_token->key);

   obj->request_token->authorisation_url = eina_stringshare_add(url);

   ecore_event_add(EBIRD_EVENT_PIN_NEED,strdup(url),NULL,NULL);

   return EINA_TRUE;
}

static Eina_Bool
_ebird_direct_token_get_cb(void *data,
                           int   type,
                           void *event_info)
{
   Async_Data *d = data;
   Ebird_Object *eobj = d->eobj;
   Ecore_Con_Event_Url_Complete *url = event_info;

   if (d->url != url->url_con)
     return EINA_TRUE;

   if (ebird_token_authenticity_get(d) < 0)
     goto error;

   DBG("[%s]\n", eobj->request_token->token);
   DBG("[%s]\n", eobj->request_token->key);
   ecore_con_url_free(d->url);
   eina_strbuf_free(d->http_data);
   d->http_data = NULL;
   d->url = NULL;
   //ebird_read_pin_from_stdin(eobj);
   ebird_authorisation_url_send(eobj);
   ebird_access_token_get(d);

error:
   return EINA_FALSE;
}

void
ebird_direct_token_get(Async_Data *d)
{
   Ecore_Event_Handler *h;
   Ebird_Object *eobj = d->eobj;
   char buf[1024];

   if (!d)
     return;

   DBG("");

   snprintf(buf, sizeof(buf),
            "%s?oauth_token=%s",
            EBIRD_DIRECT_TOKEN_URL,
            eobj->request_token->key);

   d->http_data = eina_strbuf_new();
   d->url = ecore_con_url_new(buf);
   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                               _ebird_direct_token_get_cb, d);
   DBG("eina_list_append");
   d->handlers = eina_list_append(d->handlers, h);
   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                               _url_data_cb, d);
   DBG("eina_list_append");
   d->handlers = eina_list_append(d->handlers, h);

   ecore_con_url_get(d->url);
}

static Eina_Bool
_ebird_token_request_cb(void *data,
                        int   type,
                        void *event_info)
{
   Async_Data *d = data;
   Ebird_Object *eobj = d->eobj;

   int error_code;
   int res;
   Ecore_Con_Event_Url_Complete *url = event_info;

   if (d->url != url->url_con)
     return EINA_TRUE;

   if (d->http_data)
      eobj->request_token->token = strdup(eina_strbuf_string_get(d->http_data));
   else
      return EINA_FALSE;

   DBG("request token: '%s'", eobj->request_token->token);
   if (eobj->request_token->token)
     {
        error_code = ebird_error_code_get(eobj->request_token->token);
        if ( error_code != 0)
          {
             ERR("Error code : %d", error_code);
             free(eobj->request_token->token);
             eobj->request_token->token = NULL;
          }
        else
          {
             res = oauth_split_url_parameters(eobj->request_token->token,
                                              &eobj->request_token->token_prm);

             if (res == 3)
               {
                  eobj->request_token->key =
                    strdup(&(eobj->request_token->token_prm[0][12]));

                  eobj->request_token->secret =
                    strdup(&(eobj->request_token->token_prm[1][19]));
                  eobj->request_token->callback_confirmed =
                    strdup(&(eobj->request_token->token_prm[2][25]));
               }
             else
               {
                  ERR("Error on Request Token [%s]",
                      eobj->request_token->token);
                  free(eobj->request_token->token);
                  eobj->request_token->token = NULL;
               }
          }
     }
   else
     {
        ERR("Error on Request Token [%s]", eobj->request_token->token);
        free(eobj->request_token->token);
        eobj->request_token->token = NULL;
     }

   if (eobj->account->access_token_key)
     {
        Ecore_Event_Handler *h;
        DBG("Access token exist");
        d->cb(eobj, d->data, NULL);
        ecore_con_url_free(d->url);
        eina_strbuf_free(d->http_data);
        d->http_data = NULL;
        d->url = NULL;
        EINA_LIST_FREE(d->handlers, h)
          ecore_event_handler_del(h);
        free(d);
     }
   else
     {
        Ecore_Event_Handler *h;
        DBG("Application Autorisation procedure start");
        ecore_con_url_free(d->url);
        eina_strbuf_free(d->http_data);
        d->http_data = NULL;
        d->url = NULL;
        EINA_LIST_FREE(d->handlers, h)
          ecore_event_handler_del(h);
        d->handlers = NULL;
        ebird_direct_token_get(d);
     }

   return EINA_TRUE;
}

EAPI Eina_Bool
ebird_app_authorise(Ebird_Object *eobj)
{
   printf("Ebird need to be authorised !");
   return EINA_TRUE;
}


EAPI Eina_Bool
ebird_session_open(Ebird_Object    *eobj,
                   Ebird_Session_Cb cb,
                   void            *data)
{
   Async_Data *d;
   Ecore_Event_Handler *h;

   if (!eobj)
     return EINA_FALSE;

   d = calloc(1, sizeof(Async_Data));
   if (!d)
     return EINA_FALSE;

   DBG("");

   d->eobj = eobj;
   d->cb = cb;
   d->data = data;
   d->url = ecore_con_url_new
       (ebird_oauth_sign_url
         (EBIRD_REQUEST_TOKEN_URL,
         eobj->request_token->consumer_key,
         eobj->request_token->consumer_secret,
         NULL,
         NULL,
         NULL
         )
       );

   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                               _url_data_cb, d);
   DBG("eina_list_append");
   d->handlers = eina_list_append(d->handlers, h);
   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                               _ebird_token_request_cb, d);
   DBG("eina_list_append");
   d->handlers = eina_list_append(d->handlers, h);

   ecore_con_url_get(d->url);

   return EINA_TRUE;
}

const char *
_ebird_config_dir_get(const char *suffix)
{
   char *homedir;
   const char *ret;
   Eina_Strbuf *strbuf = eina_strbuf_new();
   
   if ((homedir = getenv("HOME")))
   {
      if (suffix)
         eina_strbuf_append_printf(strbuf, "%s/.config/ebird/%s", homedir, suffix);
      else
         eina_strbuf_append_printf(strbuf, "%s/.config/ebird", homedir);
   }

   ret = eina_stringshare_add(eina_strbuf_string_get(strbuf));
   eina_strbuf_free(strbuf);
   return ret;
}

