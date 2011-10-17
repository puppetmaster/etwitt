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

static int _ebird_main_count = 0;
/* log domain variable */
int _ebird_log_dom_global = -1;

static Eina_Bool _url_data_cb(void *data, int type, void *event_info);
static Eina_Bool _url_complete_cb(void *data, int type, void *event_info);

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
   Ebird_Object *out;

   DBG("calloc of Ebird_Object :\n");
   out = calloc(1,sizeof(Ebird_Object));
   out->request_token = calloc(1,sizeof(OauthToken));
   out->account = calloc(1,sizeof(EbirdAccount));
   out->http_data = eina_strbuf_new();
   /*
   out->request_token->authenticity_token = NULL;
   out->request_token->authorisation_pin  = NULL;
   out->request_token->authorisation_url  = NULL;
   out->request_token->callback_confirmed = NULL;
   */
   ebird_id_load(out->request_token);

   return out;
}

EAPI void
ebird_del(Ebird_Object *obj)
{
    if (obj->request_token)
        free(obj->request_token);

    if (obj->account)
        free(obj->account);
}

static char *
ebird_oauth_sign_url(const char *url, Ebird_Object *obj, const char *http_method)
{
    char *out_url;

    out_url =  oauth_sign_url2(url,
                           NULL,
                           OA_HMAC,
                           http_method,
                           obj->request_token->consumer_key,
                           obj->request_token->consumer_secret,
                           obj->request_token->key,
                           obj->request_token->secret);

    DBG("[SIGNED-URL] : [%s]",out_url);

    return out_url;
}


static Eina_Bool
_url_data_cb(void *data, int type, void *event_info)
{
   Ebird_Object *obj = (Ebird_Object*)data;
       
   Ecore_Con_Event_Url_Data *url_data = event_info;

   printf("--> [%s]\n",url_data->data);
   eina_strbuf_append_length(obj->http_data, url_data->data, url_data->size);
   printf("==> [%s]\n",eina_strbuf_string_get(obj->http_data));
    return EINA_TRUE;
}

static Eina_Bool
_url_complete_cb(void *data, int type, void *event_info)
{
    Ebird_Object *obj = (Ebird_Object *)data;

   //obj->session_open(obj, obj->session_open_data); 

   ecore_con_url_free(obj->ec_url);
   ecore_event_handler_del(obj->ev_hl_data);
   ecore_event_handler_del(obj->ev_hl_complete);

    return EINA_TRUE;
}

char * 
ebird_http_get(Ebird_Object *obj, Ebird_Http_Cb cb)
{
    Eina_Strbuf *data;
    char *ret;


    obj->ec_url = ecore_con_url_new(obj->url);
    
    obj->http_complete_cb = cb;

    obj->ev_hl_data = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,_url_data_cb,obj);
    obj->ev_hl_complete = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,_url_complete_cb,obj);

    ecore_con_url_get(obj->ec_url);    

    return "NULL";
}

char * 
ebird_http_post(char *url, Ebird_Object *obj)
{
    Eina_Strbuf *data;
    int i;
    char *ret;
    char *key;
    char *value;
    char post_data[EBIRD_URL_MAX];
    char auth[EBIRD_URL_MAX];
    char **sp_url = NULL;
    char **params = NULL;

    sp_url = eina_str_split(url,"?",3);
    params = eina_str_split(sp_url[1],"&",9);


    obj->ec_url = ecore_con_url_new(sp_url[0]);
    ecore_con_url_verbose_set(obj->ec_url,EINA_TRUE);

    data = eina_strbuf_new();

    obj->ev_hl_data = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                                              _url_data_cb,data);
    obj->ev_hl_complete =  ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                                                   _url_complete_cb,NULL);

    ecore_con_url_additional_header_add(obj->ec_url,"User-Agent","Ebird 0.0.1");
    ecore_con_url_additional_header_add(obj->ec_url,"Accept","*/*");

    snprintf(auth,sizeof(auth),"OAuth ");
    for (i=0; i<=sizeof(params); i++)
    {
        key = strtok(params[i],"=");
        value = strtok('\0', "=");
        if (!strncmp("oauth",key,5))
        {
            DBG("[%i][%s]",i,key);
            if (i > 0)
                strcat(auth,", ");
            strcat(auth,key);
            strcat(auth,"=");
            strcat(auth,"\"");
            strcat(auth,value);
            strcat(auth,"\"");
        }
        if (!strcmp("status",key))
            snprintf(post_data,sizeof(post_data),"%s=%s",key,value);
            //snprintf(post_data,sizeof(post_data),"%s?%s=%s",sp_url[0],key,value);
    }

//    ecore_con_url_url_set(ec_url,post_data);

    ecore_con_url_additional_header_add(obj->ec_url,"Authorization",auth);
    ecore_con_url_additional_header_add(obj->ec_url,"Connection","close");
    ecore_con_url_additional_header_add(obj->ec_url,"Host","api.twitter.com");

    //ecore_con_url_post(ec_url," ",1,"application/x-www-form-urlencoded; charset=utf-8");
    ecore_con_url_post(obj->ec_url,post_data,strlen(post_data),
                       "application/x-www-form-urlencoded; charset=utf-8");

    //ret = strdup(eina_strbuf_string_get(data));

    return "NULL";
}

EAPI Eina_Bool
ebird_account_save(Ebird_Object *obj)
{
   Eet_File *file;
   int size;

   //DBG("DEBUG %s\n", __FUNCTION__);

   file = eet_open(EBIRD_ACCOUNT_FILE, EET_FILE_MODE_WRITE);

   eet_write(file,"username",obj->account->username,strlen(obj->account->username) + 1, 0);
   eet_write(file,"passwd",obj->account->passwd,strlen(obj->account->passwd) + 1, 1);
   eet_write(file,"access_token_key",obj->account->access_token_key,
             strlen(obj->account->access_token_key)+1,0);
   eet_write(file,"access_token_secret",obj->account->access_token_secret,
             strlen(obj->account->access_token_secret)+1, 0);
   eet_write(file,"userid",obj->account->userid,strlen(obj->account->userid)+1, 0);
   eet_write(file,"avatar",obj->account->avatar,strlen(obj->account->avatar)+1, 0);

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

   file = eet_open(EBIRD_ACCOUNT_FILE,EET_FILE_MODE_READ);
   obj->account->username = eina_stringshare_add(eet_read(file,"username",&size));
   obj->account->passwd = eina_stringshare_add(eet_read(file,"passwd",&size));
   obj->account->access_token_key = eina_stringshare_add(eet_read(file,"access_token_key",&size));
   obj->account->access_token_secret = eina_stringshare_add(eet_read(file,"access_token_secret",&size));
   obj->account->userid = eina_stringshare_add(eet_read(file,"userid",&size));

   eet_close(file);

   return EINA_TRUE;

}


EAPI int
ebird_id_load(OauthToken *request_token)
{
    Eet_File *file;
    int size;


    file = eet_open(EBIRD_ID_FILE,EET_FILE_MODE_READ);
    request_token->consumer_key = eina_stringshare_add(eet_read(file,"key",&size));
    request_token->consumer_secret = eina_stringshare_add(eet_read(file,"secret",&size));
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

    compare_res = strcmp(string,"Failed to validate oauth signature and token");
    if (compare_res == 0)
        return 500;
    else
        return 0;
}

/*
Ebird_Http_Get_Cb 
_token_request_get_cb(Ebird_Object *obj, void *data)
{
   char **res; 
	
   obj->request_token->token = obj->http_data;
   DBG("request cb token: '%s'", obj->request_token->token);
   if (obj->request_token->token)
   {
      error_code = ebird_error_code_get(obj->request_token->token);
      if ( error_code != 0)
      {
         ERR("Error code : %d", error_code);
         obj->request_token->token = NULL;
      }
      else
      {
         res = oauth_split_url_parameters(obj->request_token->token,
         &obj->request_token->token_prm);

         if (res == 3)
         {
            obj->request_token->key    = eina_stringshare_add(&(obj->request_token->token_prm[0][12]));
            obj->request_token->secret = eina_stringshare_add(&(obj->request_token->token_prm[1][19]));
            obj->request_token->callback_confirmed = eina_stringshare_add(&(obj->request_token->token_prm[2][25]));
            //DBG("DEBUG obj->request_token->key='%s', obj->request_token->secret='%s',"
            //       " obj->request_token->callback_confirmed='%s'\n",
            //       obj->request_token->key, obj->request_token->secret,
            //       obj->request_token->callback_confirmed);
         }
         else
         {
            ERR("Error on Request Token [%s]",
            obj->request_token->token);
            obj->request_token->token = NULL;
         }
      }
   }
   else
   {
      ERR("Error on Request Token [%s]",obj->request_token->token);
      obj->request_token->token = NULL;
   }
}

*/

 /*
 * name: ebird_token_request_get
 * @param : Request token variable to receive informations
 * @return : none
 * @rem : FIXME Split into 2 functions
 */
EAPI void
ebird_token_request_get(Ebird_Object *obj)
{
   int res;
   int error_code;

   obj->request_token->url = ebird_oauth_sign_url(EBIRD_REQUEST_TOKEN_URL, obj,NULL);
   obj->url = strdup(obj->request_token->url);
   ebird_http_get(obj,NULL);
   
}

int
ebird_token_authenticity_get(char *web_script, OauthToken *request_token)
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
    request_token->authenticity_token = eina_stringshare_add(key);
    *end = '\'';

    return 0;
}

int
ebird_authorisation_url_get(OauthToken *request_token)
{
    char buf[EBIRD_URL_MAX];

    snprintf(buf, sizeof(buf),
             EBIRD_DIRECT_TOKEN_URL "?authenticity_token=%s&oauth_token=%s",
             request_token->authenticity_token,
             request_token->key);

    request_token->authorisation_url = eina_stringshare_add(buf);

    return 0;
}

Eina_Bool
ebird_authorisation_pin_set(OauthToken *request_token, char *pin)
{
    request_token->authorisation_pin = strdup(pin);
    return EINA_TRUE;
}

int
ebird_authorisation_pin_get(OauthToken *request_token,
                            const char *username,
                            const char *userpassword)
{
    char *out_script;
    char *result;
    char url[EBIRD_URL_MAX];
    int retry = 4;
    int i;

    char header[EBIRD_URL_MAX];
    const char *static_header = "Accept:text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\\r\\n\
Accept-Charset:ISO-8859-1,utf-8;q=0.7,*;q=0.7\\r\\n\
Accept-Encoding:gzip, deflate\\r\\n\
Accept-Language:fr,fr-fr;q=0.8,en;q=0.5,en-us;q=0.3\\r\\n\
Connection:keep-alive\\r\\n\
Host:api.twitter.com\\r\\n\
User-Agent:Mozilla/5.0 (X11; Linux x86_64; rv:6.0.2) Gecko/20100101 Firefox/6.0.2\\r\\n";

    snprintf(header, sizeof(header),"%sRefer:%s\r\n",static_header,request_token->authorisation_url);
    snprintf(url, sizeof(url),
             "%s&session%5Busername_or_email%%5D=%s&session%%5Bpassword%%5D=%s",
             request_token->authorisation_url,
             username,
             userpassword);

    for (i = 0 ; i <= retry; i++)
    {
//        printf("\nDEBUG[ebird_authorisation_get] TRY[%i][%s]\n",i,url);
//        printf("%s\n",header);
        out_script = oauth_http_post2(url,NULL, header);
        //printf("DEBUG [%s]\n",out_script);
        if (out_script)
        {
//            printf("\nDEBUG[ebird_authorisation_get] TRY[%i][SUCCESS]\n* %s\n",i,url);

//            printf("========================================================================================================\n");
//            printf("%s\n",out_script);
//            printf("========================================================================================================\n");
            /*FIXME get the PIN */
            result = strdup("123456789");
            i = retry + 1;
        }
        else
        {
//            printf("\nDEBUG[ebird_authorisation_get] TRY[%i][FAILED][%s]\n",i, url);
            out_script = NULL;
            result = NULL;
        }
    }

    return 0;
}


int
/*ebird_access_token_get(OauthToken *request_token,
                       const char *url,
                       const char *con_key,
                       const char *con_secret,
                       EbirdAccount *account)
*/
ebird_access_token_get(Ebird_Object *obj)
{

   char *acc_url;
   char *acc_token;
   char **access_token_prm;
   char buf[EBIRD_URL_MAX];
   int res;

   access_token_prm = (char**) malloc(5*sizeof(char*));

   acc_url = ebird_oauth_sign_url(EBIRD_ACCESS_TOKEN_URL, obj, NULL);

   snprintf(buf,sizeof(buf),"%s&oauth_verifier=%s",acc_url,obj->request_token->authorisation_pin);
   obj->url = strdup(buf);
   acc_token = ebird_http_get(obj,NULL);
   DBG("DEBUG[ebird_access_token_get][URL][%s]",buf);

   if (acc_token)
   {
       DBG("DEBUG[ebird_access_token_get][RESULT]{%s}",acc_token);
       //request_token->access_token = strdup(acc_token);

       res = oauth_split_url_parameters(acc_token, &access_token_prm);
       if (res == 4)
       {
           obj->account->access_token_key = strdup(&(access_token_prm[0][12]));
           obj->account->access_token_secret = strdup(&(access_token_prm[1][19]));
           obj->account->userid = strdup(&(access_token_prm[2][8]));
           obj->account->username = strdup(&(access_token_prm[3][12]));
           obj->account->passwd = strdup("nill");
       }
       else
       {
           ERR("Error on access_token split");
           ERR("%s",acc_token);
           ERR("[%i]",res);

           return 1;
       }

   }

   free(access_token_prm);
   free(acc_url);
   return 0;
}

EAPI int
ebird_direct_token_get(Ebird_Object *obj)
{

   char *script = NULL;
   char buf[256];
   char *authenticity_token;

   snprintf(buf, sizeof(buf),
            "%s?oauth_token=%s",
            EBIRD_DIRECT_TOKEN_URL,
            obj->request_token->key);

//   printf("\nDEBUG[ebird_direct_token_get] Step[2.1][Get Authenticity token]\n");
   obj->url = strdup(buf);
   script = ebird_http_get(obj, NULL);
//   printf("get '%s'", buf);
   if (ebird_token_authenticity_get(script, obj->request_token) < 0)
       goto error;

//   printf("\nDEBUG[ebird_direct_token_get] Step[2.2][Get Authorisation page]\n");

   if (ebird_authorisation_url_get(obj->request_token) < 0)
       goto error;

   free(script);
   return 0;

error:
   free(script);
   return -1;
}

/* FIXME 
 
EAPI Eina_Bool
ebird_auto_authorise_app(Ebird_Object *obj)
{
    if (ebird_authorisation_pin_get(obj->request_token,
                                    obj->account->username,
                                    obj->account->passwd) < 0)
    {
        return EINA_FALSE;
    }

    ebird_access_token_get(obj);
    obj->account->access_token_key = strdup("xxx");
    obj->account->access_token_secret = strdup("xxx");
    return EINA_TRUE;
}
*/

Eina_Bool
ebird_read_pin_from_stdin(OauthToken *request_token)
{

    char buffer[EBIRD_PIN_SIZE];

    INF("Open this url in a web browser to authorize ebird \
            to access to your account.\n%s",
            request_token->authorisation_url);

    INF("Please paste PIN here :");
    fgets(buffer,sizeof(buffer),stdin);
    buffer[strlen(buffer)-1] = '\0';

    request_token->authorisation_pin = strdup(buffer);

    return EINA_TRUE;
}

EAPI Eina_Bool
ebird_authorise_app(Ebird_Object *obj)
{

    if (obj->request_token->authorisation_pin)
    {
//        printf("DEBUG You pin is [%s]\n",request_token->authorisation_pin);

        ebird_access_token_get(obj);

        if (ebird_account_save(obj))
            return EINA_TRUE;
        else
        {
            WRN("WARNING: Account not saved");
            return EINA_TRUE;
        }
    }
    else
    {
        ERR("Error you have to set PIN before authorising app");
        return EINA_FALSE;
    }
}

static Eina_Bool
_parse_user(void *data,Eina_Simple_XML_Type type, const char *content,
            unsigned offset, unsigned length)
{
    static EbirdAccount *cur = NULL;
    static UserState s = USER_NONE;

    data = (EbirdAccount *)data;

    if (type == EINA_SIMPLE_XML_OPEN && !strncmp("user",content,4))
    {
        if (!strncmp("screen_name", content, 11))
            s = SCREEN_NAME;
        else if (!strncmp("id",content,2))
            s = USER_ID;
        else if (!strncmp("profile_image_url_https",content, 23))
            s = AVATAR;
    }
    else if (cur && type == EINA_SIMPLE_XML_DATA)
    {
        char *ptr = strndup(content,length);
        switch(s)
        {
            case SCREEN_NAME:
                if ( ! cur->username)
                    cur->username = ptr;
                break;
            case AVATAR:
                DBG("===> [DEBUG][%s]",ptr);
                cur->avatar = ptr;
                break;
            case USER_ID:
                cur->userid = ptr;
                break;
        }

    }
    else if (cur && type == EINA_SIMPLE_XML_CLOSE && !strncmp("user",content,4))
    {
//        printf("CLOSE\n");
        data = cur;
    }
    return EINA_TRUE;

}


static Eina_Bool
_parse_timeline(void *_data, Eina_Simple_XML_Type type, const char *content, 
                unsigned offset, unsigned length)
{
    static EbirdStatus *cur = NULL;
    static State s = NONE;
    static State rt_s = NONE;
    static UserState us = USER_NONE;
    static UserState rt_us = USER_NONE;

    void **data = (void **)_data;
  //  Eina_List *timeline = (Eina_List *)data;
  //

    if (type == EINA_SIMPLE_XML_OPEN && !strncmp("status",content,length))
    {
        cur = calloc(1,sizeof(EbirdStatus));
        cur->user = calloc(1,sizeof(EbirdAccount));
        //cur->retweeted = EINA_FALSE;
    }
    else if (cur && type == EINA_SIMPLE_XML_OPEN)
    {
        us = USER_NONE;
        if (!strncmp("retweeted_status",content,16))
        {
            s = RETWEETED;
 //           printf("[-=O=-]RETWEETED[-=O=-]\n");
            cur->retweeted_status = calloc(1,sizeof(EbirdStatus));
            cur->retweeted_status->user = calloc(1,sizeof(EbirdAccount));
            cur->retweeted = EINA_TRUE;
        }
        else if (!strncmp("created_at", content, 10))
            s = CREATEDAT;
        else if (!strncmp("text",content,4))
            s = TEXT;
        else if (!strncmp("id",content,2))
            s = ID;
        else if (!strncmp("user",content,4))
            s = USER;
        else if (!strncmp("screen_name",content, 11))
        {
            us = SCREEN_NAME;
            s = USER;
        }
        else
            s = NONE;
    }
    else if (cur && type == EINA_SIMPLE_XML_DATA)
    {
        char *ptr = strndup(content,length);
        if ( ! cur->retweeted)
        {
            switch(s)
            {
                case CREATEDAT:
                    cur->created_at = ptr;
                    break;
                case TEXT:
                    //                printf("===> [DEBUG][%s]\n",ptr);
                    cur->text = ptr;
                    break;
                case ID:
                    cur->id = ptr;
                    break;
            }
            switch(us)
            {
                case SCREEN_NAME:
                    //printf("DEBUG ICI [%d][%s]\n",us,ptr);
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
                    //printf("===> [DEBUG][%s]\n",ptr);
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
//                    printf("DEBUG ICI [%d][%s]\n",us,ptr);
                    cur->retweeted_status->user->username = ptr;
                    break;
                case USER_NONE:
                    break;
            }
        }

    }
    else if (cur && type == EINA_SIMPLE_XML_CLOSE && !strncmp("status",content,length))
    {
//        printf("DEBUG APPEND\n");
//        printf("DEBUG [%s]\n",cur->created_at);
//        data = eina_list_append(data,cur);
        if (!strncmp("RT ",cur->text,3))
            cur->retweeted = EINA_TRUE;
        *data = eina_list_append(*data,cur);
        cur = NULL;
    }
    else if (cur && type == EINA_SIMPLE_XML_CLOSE && !strncmp("retweeted_status",content,16))
    {
//        printf("CLOSE\n");
        if (cur->retweeted)
            cur->retweeted = EINA_FALSE;
    }
    return EINA_TRUE;
}

EAPI void
ebird_timeline_free(Eina_List *timeline)
{
    EbirdStatus *st;

    EINA_LIST_FREE(timeline,st)
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

EAPI Eina_List *
ebird_timeline_get(const char *url, Ebird_Object *obj)
{

    char *xml;
    char *timeline_url;
    Eina_List *timeline = NULL;
    Eina_Simple_XML_Node_Root *root;

    timeline_url =  ebird_oauth_sign_url(url, obj, NULL);
   obj->url = strdup(timeline_url);
    xml = ebird_http_get(obj,NULL);
//    printf("\n\n%s\n\n",xml);
    eina_simple_xml_parse(xml,strlen(xml),EINA_TRUE,_parse_timeline, &timeline);
    return timeline;
}

EAPI Eina_List *
ebird_timeline_home_get(Ebird_Object *obj)
{

    Eina_List *timeline;

    timeline = ebird_timeline_get(EBIRD_HOME_TIMELINE_URL, obj);

    return timeline;
}

EAPI Eina_List *
ebird_timeline_public_get(Ebird_Object *obj)
{

    Eina_List *timeline;

    timeline = ebird_timeline_get(EBIRD_PUBLIC_TIMELINE_URL, obj);
    return timeline;
}

EAPI Eina_List *
ebird_timeline_user_get(Ebird_Object *obj)
{
    Eina_List *timeline;

    timeline = ebird_timeline_get(EBIRD_USER_TIMELINE_URL, obj);
    return timeline;
}

EAPI Eina_List *
ebird_timeline_mentions_get(Ebird_Object *obj)
{
    Eina_List *mentions;

    mentions = ebird_timeline_get(EBIRD_USER_MENTIONS_URL, obj);
    return mentions;
}


char *
ebird_home_timeline_xml_get(Ebird_Object *obj)
{

   char *xml_timeline;
   char *timeline_url;

   timeline_url =  ebird_oauth_sign_url(EBIRD_HOME_TIMELINE_URL,obj, NULL);
   obj->url = strdup(timeline_url);
   xml_timeline = ebird_http_get(obj,NULL);
//FIXME FREE timeline_url;
   return xml_timeline;
}

EAPI Eina_Bool
ebird_user_sync(Ebird_Object *obj)
{
    char buf[EBIRD_URL_MAX];
    char *infos;
    char *url;

    url = strdup(EBIRD_USER_SHOW_URL);

    snprintf(buf,sizeof(buf),"%s&screen_name=%s&userid=%s",
             url,
             obj->account->username,
             obj->account->userid);
   obj->url = strdup(buf);
    infos = ebird_http_get(obj,NULL);
    DBG("DEBUG [%s]\n[%s]",buf,infos);
    eina_simple_xml_parse(infos,strlen(infos),EINA_TRUE,_parse_user, &(obj->account));
    free(url);
    return EINA_TRUE;
}

EAPI EbirdAccount *
ebird_user_get(char *username,Ebird_Object *obj)
{
    char buf[EBIRD_URL_MAX];
    char *infos;
    char *url;

    EbirdAccount *user;

    url = strdup(EBIRD_USER_SHOW_URL);

    snprintf(buf,sizeof(buf),"%s&screen_name=%s",
             url,
             user->username);

   obj->url = strdup(buf);
   infos = ebird_http_get(obj,NULL);
   eina_simple_xml_parse(infos,strlen(infos),EINA_TRUE,_parse_user, &user);
   free(url);
   return user;
}

EAPI Eina_Bool
ebird_user_show(EbirdAccount *acc)
{
    if (acc)
    {
        if (acc->username)
            INF("USERNAME : [%s]",acc->username);
        if (acc->userid)
            INF("USERID : [%s]",acc->userid);
        if (acc->avatar)
            INF("AVATAR : [%s]",acc->avatar);
    }
    else
    {
        WRN("Nothing to show for this user");
        return EINA_FALSE;
    }

    return EINA_TRUE;

}

EAPI char *
ebird_credentials_verify(Ebird_Object *obj)
{
    char *url;
    char *ret;

    obj->url = ebird_oauth_sign_url(EBIRD_ACCOUNT_CREDENTIALS_URL, obj, NULL);
    ret = ebird_http_get(obj,NULL);
    return ret;
}

EAPI Eina_Bool
ebird_status_update(Ebird_Object *obj, char *message)
{
    char *url;
    char *ret;
    char up_url[EBIRD_URL_MAX];

    if (!obj)
        return EINA_FALSE;

    url = ebird_oauth_sign_url(EBIRD_STATUS_URL, obj,"POST");

    snprintf(up_url,sizeof(up_url),"%s&status=%s&include_entities=true",url,message);
//    printf("DEBUG\n[>%s<]\n",up_url);
    ret = ebird_http_post(up_url,obj);
    DBG("\n%s\n\n",ret);
    return EINA_TRUE;

}

EAPI Eina_Bool
ebird_session_open(Ebird_Object *obj, Ebird_Session_Cb cb, void *data)
{

    if (!obj)
        return EINA_FALSE;

    obj->session_open = cb;
    obj->session_open_data = data;

//    ebird_token_request_get(obj,_token_request_get_cb,);
    ebird_token_request_get(obj);
    if (obj->request_token->token)
    {
        if (obj->account->access_token_key)
        {
            return EINA_TRUE;
        }
        else
        {
            ebird_direct_token_get(obj);
            ebird_read_pin_from_stdin(obj->request_token);
            ebird_authorise_app(obj);
            return EINA_TRUE;
        }
    }
    else
    {
        ERR("Error opening session\n");
        return EINA_FALSE;
    }

}



static Eina_Bool
_ebird_direct_token_get(void *data, int type, void *event_info)
{
   Ebird_Object *obj = (Ebird_Object *)data;


   puts("DEBUG\n");
   printf("[%s]\n",obj->request_token->token); 
   return EINA_TRUE;
}
   
static Eina_Bool
_ebird_token_request_get(void *data, int type, void *event_info)
{
   
   Ebird_Object *obj = (Ebird_Object *)data;
   int error_code;
   int res;
   
   obj->request_token->token = eina_strbuf_string_get(obj->http_data);
   
   DBG("request token: '%s'", obj->request_token->token);
   if (obj->request_token->token)
   {
        error_code = ebird_error_code_get(obj->request_token->token);
        if ( error_code != 0)
        {
            ERR("Error code : %d", error_code);
            obj->request_token->token = NULL;
        }
        else
        {
            res = oauth_split_url_parameters(obj->request_token->token,
                                             &obj->request_token->token_prm);

            if (res == 3)
            {
               obj->request_token->key = 
                  eina_stringshare_add(&(obj->request_token->token_prm[0][12]));
                                           
               obj->request_token->secret = 
                  eina_stringshare_add(&(obj->request_token->token_prm[1][19]));
               obj->request_token->callback_confirmed = 
                  eina_stringshare_add(&(obj->request_token->token_prm[2][25]));
                //DBG("DEBUG request->key='%s', request->secret='%s',"
                // " request->callback_confirmed='%s'\n",
                // request->key, request->secret,
                // request->callback_confirmed);
            }
            else
            {
                ERR("Error on Request Token [%s]",
                       obj->request_token->token);
                obj->request_token->token = NULL;
            }
        }
    }
    else
    {
        ERR("Error on Request Token [%s]",obj->request_token->token);
        obj->request_token->token = NULL;
    }
   

   obj->http_complete_cb(obj,type,event_info);
   
   ecore_con_url_free(obj->ec_url);
   ecore_event_handler_del(obj->ev_hl_data);
   ecore_event_handler_del(obj->ev_hl_complete);
   return EINA_TRUE;

}

char * 
ebird_http_get2(Ebird_Object *obj)
{
   Eina_Strbuf *data;
   char *ret;

    obj->ec_url = ecore_con_url_new(obj->url);
    obj->http_data = eina_strbuf_new();

    obj->ev_hl_data = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,_url_data_cb,obj);
    obj->ev_hl_complete = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,(Ecore_Event_Handler_Cb)obj->http_complete_cb,obj);

    ecore_con_url_get(obj->ec_url);    

    return "NULL";
}

EAPI Eina_Bool
ebird_session_open2(Ebird_Object *obj, Ebird_Session_Cb cb, void *data)
{
   if (!obj)
      return EINA_FALSE;
    
   obj->request_token->url = ebird_oauth_sign_url(EBIRD_REQUEST_TOKEN_URL, obj,NULL);
   obj->url = strdup(obj->request_token->url);
   obj->http_complete_cb = &_ebird_direct_token_get;
   
   ebird_http_get2(obj);   
   
   return EINA_TRUE;
}
