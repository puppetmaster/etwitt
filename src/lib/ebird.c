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

EAPI Eina_Bool
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

EAPI Ebird_Object
*ebird_add()
{
    Ebird_Object *out;

    DBG("calloc of Ebird_Object :\n");
    out = calloc(1,sizeof(Ebird_Object));
    out->request_token = calloc(1,sizeof(OauthToken));
    out->account = calloc(1,sizeof(EbirdAccount));

    ebird_id_load(out->request_token);

    return out;
}

EAPI void
ebird_object_free(Ebird_Object *obj)
{
    if (obj->request_token)
        free(obj->request_token);

    if (obj->account)
        free(obj->account);
}

static char *
ebird_oauth_sign_url(const char *url,
                     const char *consumer_key,
                     const char *consumer_secret,
                     const char *token_key,
                     const char *token_secret,
                     const char *http_method)
{
    char *out_url;

    out_url =  oauth_sign_url2(url,
                           NULL,
                           OA_HMAC,
                           http_method,
                           consumer_key,
                           consumer_secret,
                           token_key,
                           token_secret);

    DBG("[SIGNED-URL] : [%s]",out_url);

    return out_url;
}


static Eina_Bool
_url_data_cb(void *data, int type, void *event_info)
{
    Ecore_Con_Event_Url_Data *url_data = event_info;

    eina_strbuf_append_length(data, url_data->data,url_data->size);

    return EINA_TRUE;
}

static Eina_Bool
_url_complete_cb(void *data, int type, void *event_info)
{
    ecore_main_loop_quit();
    return EINA_TRUE;
}

char *
ebird_http_get(char *url)
{
    Ecore_Con_Url *ec_url;
    Eina_Strbuf *data;
    char *ret;


    ec_url = ecore_con_url_new(url);
    data = eina_strbuf_new();

    ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,_url_data_cb,data);
    ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,_url_complete_cb,NULL);

    ecore_con_url_get(ec_url);

    ecore_main_loop_begin();

    ecore_con_url_free(ec_url);

    ret = strdup(eina_strbuf_string_get(data));

    return ret;
}

char *
ebird_http_post(char *url)
{
    Ecore_Con_Url *ec_url;
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


    ec_url = ecore_con_url_new(sp_url[0]);
    ecore_con_url_verbose_set(ec_url,EINA_TRUE);

    data = eina_strbuf_new();

    ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,_url_data_cb,data);
    ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,_url_complete_cb,NULL);

    ecore_con_url_additional_header_add(ec_url,"User-Agent","Ebird 0.0.1");
    ecore_con_url_additional_header_add(ec_url,"Accept","*/*");

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

    ecore_con_url_additional_header_add(ec_url,"Authorization",auth);
    ecore_con_url_additional_header_add(ec_url,"Connection","close");
    ecore_con_url_additional_header_add(ec_url,"Host","api.twitter.com");

    //ecore_con_url_post(ec_url," ",1,"application/x-www-form-urlencoded; charset=utf-8");
    ecore_con_url_post(ec_url,post_data,strlen(post_data),"application/x-www-form-urlencoded; charset=utf-8");

    ecore_main_loop_begin();

    ecore_con_url_free(ec_url);

    ret = strdup(eina_strbuf_string_get(data));

    return ret;
}

EAPI Eina_Bool
ebird_account_save(EbirdAccount *account)
{
   Eet_File *file;
   int size;

   //DBG("DEBUG %s\n", __FUNCTION__);

   file = eet_open(EBIRD_ACCOUNT_FILE, EET_FILE_MODE_WRITE);

   eet_write(file,"username",account->username,strlen(account->username) + 1, 0);
   eet_write(file,"passwd",account->passwd,strlen(account->passwd) + 1, 1);
   eet_write(file,"access_token_key",account->access_token_key,
             strlen(account->access_token_key)+1,0);
   eet_write(file,"access_token_secret",account->access_token_secret,
             strlen(account->access_token_secret)+1, 0);
   eet_write(file,"userid",account->userid,strlen(account->userid)+1, 0);
   eet_write(file,"avatar",account->avatar,strlen(account->avatar)+1, 0);

   eet_close(file);


   return EINA_TRUE;
}

EAPI Eina_Bool
ebird_account_load(EbirdAccount *account)
{
   Eet_File *file;
   int size;


   file = eet_open(EBIRD_ACCOUNT_FILE,EET_FILE_MODE_READ);
   account->username = eina_stringshare_add(eet_read(file,"username",&size));
   account->passwd = eina_stringshare_add(eet_read(file,"passwd",&size));
   account->access_token_key = eina_stringshare_add(eet_read(file,"access_token_key",&size));
   account->access_token_secret = eina_stringshare_add(eet_read(file,"access_token_secret",&size));
   account->userid = eina_stringshare_add(eet_read(file,"userid",&size));

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
 * name: ebird_token_request_get
 * @param : Request token variable to receive informations
 * @return : none
 * @rem : FIXME Split into 2 functions
 */
EAPI void
ebird_token_request_get(OauthToken *request)
{
    int res;
    int error_code;

    request->url = ebird_oauth_sign_url(EBIRD_REQUEST_TOKEN_URL,
                                        request->consumer_key,
                                        request->consumer_secret, NULL, NULL, NULL);
    request->token = ebird_http_get(request->url);
    DBG("request token: '%s'", request->token);
    if (request->token)
    {
        error_code = ebird_error_code_get(request->token);
        if ( error_code != 0)
        {
            ERR("Error code : %d", error_code);
            request->token = NULL;
        }
        else
        {
            res = oauth_split_url_parameters(request->token,
                                             &request->token_prm);

            if (res == 3)
            {
                request->key = eina_stringshare_add(&(request->token_prm[0][12]));
                request->secret = eina_stringshare_add(&(request->token_prm[1][19]));
                request->callback_confirmed = eina_stringshare_add(&(request->token_prm[2][25]));
                //DBG("DEBUG request->key='%s', request->secret='%s',"
                //       " request->callback_confirmed='%s'\n",
                //       request->key, request->secret,
                //       request->callback_confirmed);
            }
            else
            {
                ERR("Error on Request Token [%s]",
                       request->token);
                request->token = NULL;
            }
        }
    }
    else
    {
        ERR("Error on Request Token [%s]",request->token);
        request->token = NULL;
    }
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
ebird_access_token_get(OauthToken *request_token,
                       const char *url,
                       const char *con_key,
                       const char *con_secret,
                       EbirdAccount *account)
{

   char *acc_url;
   char *acc_token;
   char **access_token_prm;
   char buf[EBIRD_URL_MAX];
   int res;

   access_token_prm = (char**) malloc(5*sizeof(char*));

   acc_url = ebird_oauth_sign_url(url,con_key,con_secret,
                                  request_token->key,
                                  request_token->secret,NULL);

   snprintf(buf,sizeof(buf),"%s&oauth_verifier=%s",acc_url,request_token->authorisation_pin);

   acc_token = ebird_http_get(buf);
   DBG("DEBUG[ebird_access_token_get][URL][%s]",buf);

   if (acc_token)
   {
       DBG("DEBUG[ebird_access_token_get][RESULT]{%s}",acc_token);
       //request_token->access_token = strdup(acc_token);

       res = oauth_split_url_parameters(acc_token, &access_token_prm);
       if (res == 4)
       {
           account->access_token_key = strdup(&(access_token_prm[0][12]));
           account->access_token_secret = strdup(&(access_token_prm[1][19]));
           account->userid = strdup(&(access_token_prm[2][8]));
           account->username = strdup(&(access_token_prm[3][12]));
           account->passwd = strdup("nill");
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
ebird_direct_token_get(OauthToken *request_token)
{

   char *script = NULL;
   char buf[256];
   char *authenticity_token;

   snprintf(buf, sizeof(buf),
            "%s?oauth_token=%s",
            EBIRD_DIRECT_TOKEN_URL,
            request_token->key);

//   printf("\nDEBUG[ebird_direct_token_get] Step[2.1][Get Authenticity token]\n");
   script = ebird_http_get(buf);
//   printf("get '%s'", buf);
   if (ebird_token_authenticity_get(script, request_token) < 0)
       goto error;

//   printf("\nDEBUG[ebird_direct_token_get] Step[2.2][Get Authorisation page]\n");

   if (ebird_authorisation_url_get(request_token) < 0)
       goto error;

   free(script);
   return 0;

error:
   free(script);
   return -1;
}

EAPI Eina_Bool
ebird_auto_authorise_app(OauthToken *request_token, EbirdAccount *account)
{
    if (ebird_authorisation_pin_get(request_token,
                                    account->username,
                                    account->passwd) < 0)
    {
        return EINA_FALSE;
    }

    ebird_access_token_get(request_token,
            EBIRD_ACCESS_TOKEN_URL,
            request_token->consumer_key,
            request_token->consumer_secret,
            account);
    account->access_token_key = strdup("xxx");
    account->access_token_secret = strdup("xxx");
    return EINA_TRUE;
}

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
ebird_authorise_app(OauthToken *request_token, EbirdAccount *account)
{

    if (request_token->authorisation_pin)
    {
//        printf("DEBUG You pin is [%s]\n",request_token->authorisation_pin);

        ebird_access_token_get(request_token,
                EBIRD_ACCESS_TOKEN_URL,
                request_token->consumer_key,
                request_token->consumer_secret,
                account);

        if (ebird_account_save(account))
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
_parse_user(void *data,Eina_Simple_XML_Type type, const char *content,unsigned offset, unsigned length)
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
_parse_timeline(void *_data, Eina_Simple_XML_Type type, const char *content, unsigned offset, unsigned length)
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
    Eina_List *l;

    EINA_LIST_FOREACH(timeline,l,st)
    {
        if (st->retweeted)
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
    eina_list_free(timeline);
}

EAPI Eina_List *
ebird_timeline_get(const char *url, OauthToken *request, EbirdAccount *acc)
{

    char *xml;
    char *timeline_url;
    Eina_List *timeline = NULL;
    Eina_Simple_XML_Node_Root *root;

    timeline_url =  ebird_oauth_sign_url(url,
                                request->consumer_key,
                                request->consumer_secret,
                                acc->access_token_key,
                                acc->access_token_secret,NULL);

    xml = ebird_http_get(timeline_url);
//    printf("\n\n%s\n\n",xml);
    eina_simple_xml_parse(xml,strlen(xml),EINA_TRUE,_parse_timeline, &timeline);
    return timeline;
}

EAPI Eina_List *
ebird_home_timeline_get(Ebird_Object *obj)
{

    Eina_List *timeline;

    timeline = ebird_timeline_get(EBIRD_HOME_TIMELINE_URL,
                                  obj->request_token,
                                  obj->account);
    return timeline; 
}

EAPI Eina_List *
ebird_public_timeline_get(Ebird_Object *obj)
{

    Eina_List *timeline;

    timeline = ebird_timeline_get(EBIRD_PUBLIC_TIMELINE_URL,
                                  obj->request_token,
                                  obj->account);
    return timeline; 
}

EAPI Eina_List *
ebird_user_timeline_get(Ebird_Object *obj)
{
    Eina_List *timeline;
    
    timeline = ebird_timeline_get(EBIRD_USER_TIMELINE_URL,
                                  obj->request_token,
                                  obj->account);
    return timeline;
}

EAPI Eina_List *
ebird_user_mentions_get(OauthToken *request, EbirdAccount *acc)
{
    Eina_List *mentions;
    
    mentions = ebird_timeline_get(EBIRD_USER_MENTIONS_URL,request,acc);
    return mentions;
}


char *
ebird_home_timeline_xml_get(OauthToken *request, EbirdAccount *acc)
{

    char *xml_timeline;
    char *timeline_url;

    timeline_url =  ebird_oauth_sign_url(EBIRD_HOME_TIMELINE_URL,
                                        request->consumer_key,
                                        request->consumer_secret,
                                        acc->access_token_key,
                                        acc->access_token_secret,NULL);

    xml_timeline = ebird_http_get(timeline_url);
//FIXME FREE timeline_url;        
    return xml_timeline;
}

EAPI Eina_Bool
ebird_user_sync(EbirdAccount *user)
{
    char buf[EBIRD_URL_MAX];
    char *infos;
    char *url;

    url = strdup(EBIRD_USER_SHOW_URL);

    snprintf(buf,sizeof(buf),"%s&screen_name=%s&userid=%s",
             url,
             user->username,
             user->userid);

    infos = ebird_http_get(buf);
    DBG("DEBUG [%s]\n[%s]",buf,infos);
    eina_simple_xml_parse(infos,strlen(infos),EINA_TRUE,_parse_user, &user);
    free(url);
    return EINA_TRUE;
}

EAPI EbirdAccount *
ebird_user_get(char *username)
{
    char buf[EBIRD_URL_MAX];
    char *infos;
    char *url;

    EbirdAccount *user;

    url = strdup(EBIRD_USER_SHOW_URL);

    snprintf(buf,sizeof(buf),"%s&screen_name=%s",
             url,
             user->username);

    infos = ebird_http_get(buf);
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

    url = ebird_oauth_sign_url(EBIRD_ACCOUNT_CREDENTIALS_URL,
                               obj->request_token->consumer_key,
                               obj->request_token->consumer_secret,
                               obj->account->access_token_key,
                               obj->account->access_token_secret, NULL);
    ret = ebird_http_get(url);
    return ret;
}

EAPI Eina_Bool
ebird_update_status(char *message,OauthToken *request,EbirdAccount *acc)
{
    char *url;
    char *ret;
    char up_url[EBIRD_URL_MAX];

    url = ebird_oauth_sign_url(EBIRD_STATUS_URL,
                               request->consumer_key,
                               request->consumer_secret,
                               acc->access_token_key,
                               acc->access_token_secret,"POST");

    snprintf(up_url,sizeof(up_url),"%s&status=%s&include_entities=true",url,message);
//    printf("DEBUG\n[>%s<]\n",up_url);
    ret = ebird_http_post(up_url);
    DBG("\n%s\n\n",ret);
    return EINA_TRUE;

}

EAPI Eina_Bool
ebird_session_open(OauthToken *request_token, EbirdAccount *account)
{

    ebird_token_request_get(request_token);
    if (request_token->token)
    {
        if (account->access_token_key)
        {
            return EINA_TRUE;
        }
        else
        {
            ebird_direct_token_get(request_token);
            ebird_read_pin_from_stdin(request_token);
            ebird_authorise_app(request_token,account);
            return EINA_TRUE;
        }
    }
    else
    {
        ERR("Error opening session\n");
        return EINA_FALSE;
    }

}
