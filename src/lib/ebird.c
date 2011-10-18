#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <oauth.h>

#include <Eina.h>
#include <Eet.h>
#include <Ecore.h>
#include <Ecore_Con.h>

#include "Ebird.h"
#include "ebird_private.h"

typedef struct _Async_Data Async_Data;

struct _Async_Data
{
   Ebird_Object  *eobj;
   void           (*cb)(Ebird_Object *obj,
                        void         *data);
   void          *data;
   Ecore_Con_Url *url;
   Eina_Strbuf   *http_data;
   Eina_List     *handlers;
};

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

EAPI int
ebird_init()
{
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
   if (!eet_init())
     goto shutdown_ecore_con_url;

   _ebird_log_dom_global = eina_log_domain_register("ebird", EBIRD_DEFAULT_LOG_COLOR);
   if (_ebird_log_dom_global < 0)
     {
        EINA_LOG_ERR("Ebird Can not create a general log domain.");
        goto shutdown_ecore_con_url;
     }

   DBG("Ebird Init done");

   _ebird_main_count = 1;
   return 1;

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

EAPI int
ebird_shutdown()
{
   _ebird_main_count--;
   if (EINA_UNLIKELY(_ebird_main_count == 0))
     {
        eet_shutdown();
        ecore_con_url_shutdown();
        ecore_con_shutdown();
        ecore_shutdown();
        eina_shutdown();
     }
   return _ebird_main_count;
}

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

EAPI void
ebird_del(Ebird_Object *eobj)
{
   if (eobj->request_token)
     free(eobj->request_token);

   if (eobj->account)
     free(eobj->account);
}

static char *
ebird_oauth_sign_url(const char   *url,
                     Ebird_Object *obj,
                     const char   *http_method)
{
   char *out_url;

   out_url = oauth_sign_url2(url,
                             NULL,
                             OA_HMAC,
                             http_method,
                             obj->request_token->consumer_key,
                             obj->request_token->consumer_secret,
                             obj->request_token->key,
                             obj->request_token->secret);

   DBG("[SIGNED-URL] : [%s]", out_url);

   return out_url;
}

static Eina_Bool
_url_data_cb(void *data,
             int   type,
             void *event_info)
{
   Async_Data *d = data;
   Ecore_Con_Event_Url_Data *url_data = event_info;

   if (!d->http_data)
     d->http_data = eina_strbuf_new();

   eina_strbuf_append_length(d->http_data,
                             url_data->data,
                             url_data->size);
   DBG("==> [%s]", eina_strbuf_string_get(d->http_data));
   return EINA_TRUE;
}

EAPI Eina_Bool
ebird_account_save(Ebird_Object *obj)
{
   Eet_File *file;
   int size;

   file = eet_open(EBIRD_ACCOUNT_FILE, EET_FILE_MODE_WRITE);

   eet_write(file, "username", obj->account->username, strlen(obj->account->username) + 1, 0);
   eet_write(file, "passwd", obj->account->passwd, strlen(obj->account->passwd) + 1, 1);
   eet_write(file, "access_token_key", obj->account->access_token_key,
             strlen(obj->account->access_token_key) + 1, 0);
   eet_write(file, "access_token_secret", obj->account->access_token_secret,
             strlen(obj->account->access_token_secret) + 1, 0);
   eet_write(file, "userid", obj->account->userid, strlen(obj->account->userid) + 1, 0);
   eet_write(file, "avatar", obj->account->avatar, strlen(obj->account->avatar) + 1, 0);

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

   file = eet_open(EBIRD_ACCOUNT_FILE, EET_FILE_MODE_READ);
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

   file = eet_open(EBIRD_ID_FILE, EET_FILE_MODE_READ);
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
ebird_token_authenticity_get(char       *web_script,
                             OauthToken *request_token)
{
   char *key,
   *end;

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
   request_token->authenticity_token = strdup(key);
   *end = '\'';

   return 0;
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

   data = (EbirdAccount *)data;

   if (type == EINA_SIMPLE_XML_OPEN && !strncmp("user", content, 4))
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
             cur->avatar = ptr;
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
               }
             switch(us)
               {
                case SCREEN_NAME:
                  cur->user->username = ptr;
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
               }

             switch(us)
               {
                case SCREEN_NAME:
                  cur->retweeted_status->user->username = ptr;
                  break;

                case USER_NONE:
                  break;
               }
          }
     }
   else if (cur && type == EINA_SIMPLE_XML_CLOSE && !strncmp("status", content, length))
     {
        if (!strncmp("RT ", cur->text, 3))
          cur->retweeted = EINA_TRUE;
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
   Async_Data *d = data;
   Ebird_Object *eobj = d->eobj;
   const char *xml = eina_strbuf_string_get(d->http_data);
   Eina_List *timeline = NULL;
   eina_simple_xml_parse(xml, strlen(xml), EINA_TRUE, _parse_timeline, &timeline);

   if (d->cb)
     d->cb(eobj, d->data);
}

static void
ebird_timeline_get(const char *url,
                   Async_Data *d)
{
   Ecore_Event_Handler *h;
   Ebird_Object *eobj = d->eobj;

   d->url = ecore_con_url_new(ebird_oauth_sign_url(url, eobj, NULL));
   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                               _url_data_cb, d);
   d->handlers = eina_list_append(d->handlers, h);
   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                               _ebird_timeline_get_cb, d);
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
ebird_timeline_public_get(Ebird_Object *obj)
{
   ebird_timeline_get(EBIRD_PUBLIC_TIMELINE_URL, NULL);
}

EAPI void
ebird_timeline_user_get(Ebird_Object *obj)
{
   ebird_timeline_get(EBIRD_USER_TIMELINE_URL, NULL);
}

EAPI void
ebird_timeline_mentions_get(Ebird_Object *obj)
{
   ebird_timeline_get(EBIRD_USER_MENTIONS_URL, NULL);
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
_ebird_direct_token_get_cb(void *data,
                           int   type,
                           void *event_info)
{
   Async_Data *d = data;
   Ebird_Object *eobj = d->eobj;

   DBG("");
   DBG("[%s]\n", eobj->request_token->token);
   DBG("[%s]\n", eobj->request_token->key);
}

static void
ebird_direct_token_get2(Async_Data *d)
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
   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, _ebird_direct_token_get_cb, d);
   d->handlers = eina_list_append(d->handlers, h);
   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                               _url_data_cb, d);
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

   eobj->request_token->token = strdup(eina_strbuf_string_get(d->http_data));

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
        d->cb(eobj, d->data);
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
        ebird_direct_token_get2(d);
     }

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
         eobj,
         NULL
         )
       );

   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                               _url_data_cb, d);
   d->handlers = eina_list_append(d->handlers, h);
   h = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                               _ebird_token_request_cb, d);
   d->handlers = eina_list_append(d->handlers, h);

   ecore_con_url_get(d->url);

   return EINA_TRUE;
}

