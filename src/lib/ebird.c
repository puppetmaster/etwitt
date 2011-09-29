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

#include "ebird.h"

Eina_Bool
ebird_init()
{
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

    return EINA_TRUE;

 shutdown_ecore_con_url:
    ecore_con_url_shutdown();
 shutdown_ecore_con:
    ecore_con_shutdown();
 shutdown_ecore:
    ecore_shutdown();
 shutdown_eina:
    eina_shutdown();

    return EINA_FALSE;
}

void
ebird_shutdown()
{
    eet_shutdown();
    ecore_con_url_shutdown();
    ecore_con_shutdown();
    ecore_shutdown();
    eina_shutdown();
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

Eina_Bool
ebird_save_account(EbirdAccount *account)
{
   Eet_File *file;
   int size;

//   printf("DEBUG ebird_save_account\n");

   file = eet_open(EBIRD_ACCOUNT_FILE, EET_FILE_MODE_WRITE);

   eet_write(file,"username",account->username,strlen(account->username) + 1, 0);
   eet_write(file,"passwd",account->passwd,strlen(account->passwd) + 1, 1);
   eet_write(file,"access_token_key",account->access_token_key,
             strlen(account->access_token_key)+1,0);
   eet_write(file,"access_token_secret",account->access_token_secret,
             strlen(account->access_token_secret)+1, 0);
   eet_write(file,"userid",account->userid,strlen(account->userid)+1, 0);

   eet_close(file);


   return EINA_TRUE;
}

Eina_Bool
ebird_load_account(EbirdAccount *account)
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


int
ebird_load_id(OauthToken *request_token)
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
 * name: ebird_request_token_get
 * @param : Request token variable to receive informations
 * @return : none
 * @rem : FIXME Split into 2 functions
 */
void
ebird_request_token_get(OauthToken *request)
{
    int res;
    int error_code;

    request->url = oauth_sign_url2(EBIRD_REQUEST_TOKEN_URL, NULL, OA_HMAC,
                                   NULL, request->consumer_key,
                                   request->consumer_secret, NULL, NULL);
    request->token = ebird_http_get(request->url);
//    printf("DEBUG request token: '%s'\n", request->token);
    if (request->token)
    {
        error_code = ebird_error_code_get(request->token);
        if ( error_code != 0)
        {
            printf("Error !\n");
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
                //printf("DEBUG request->key='%s', request->secret='%s',"
                //       " request->callback_confirmed='%s'\n",
                //       request->key, request->secret,
                //       request->callback_confirmed);
            }
            else
            {
                printf("Error on Request Token [%s]\n",
                       request->token);
                request->token = NULL;
            }
        }
    }
    else
    {
//        printf("\nDEBUG : [%s]\n",request->url);
        printf("Error on Request Token [%s]\n",request->token);
        request->token = NULL;
    }
}

int
ebird_authenticity_token_get(char *web_script, OauthToken *request_token)
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

   acc_url = oauth_sign_url2(url, NULL, OA_HMAC, NULL,
                             con_key,
                             con_secret,
                             request_token->key,
                             request_token->secret);

   snprintf(buf,sizeof(buf),"%s&oauth_verifier=%s",acc_url,request_token->authorisation_pin);

   acc_token = ebird_http_get(buf);
//   printf("\nDEBUG[ebird_access_token_get][URL][%s]\n",buf);

   if (acc_token)
   {
//       printf("\nDEBUG[ebird_access_token_get][RESULT]{%s}\n",acc_token);
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
           printf("Error on access_token split\n");
           printf("%s\n",acc_token);
           printf("[%i]\n",res);

           return 1;
       }

   }

   free(access_token_prm);
   free(acc_url);
   return 0;
}

int
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
   if (ebird_authenticity_token_get(script, request_token) < 0)
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

Eina_Bool
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

    printf("Open this url in a web browser to authorize ebird \
            to access to your account.\n%s\n",
            request_token->authorisation_url);

    printf("Please paste PIN here :\n");
    fgets(buffer,sizeof(buffer),stdin);
    buffer[strlen(buffer)-1] = '\0';

    request_token->authorisation_pin = strdup(buffer);

    return EINA_TRUE;
}

Eina_Bool
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

        if (ebird_save_account(account))
            return EINA_TRUE;
        else
        {
            printf("WARNING: Account not saved\n");
            return EINA_TRUE;
        }
    }
    else
    {
        printf("Error you have to set PIN before authorising app\n");
        return EINA_FALSE;
    }
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
    
    puts("01");
    if (type == EINA_SIMPLE_XML_OPEN && !strncmp("status",content,length))
        cur = calloc(1,sizeof(EbirdStatus));
    else if (cur && type == EINA_SIMPLE_XML_OPEN) 
    {
        if ( ! cur->retweeted)
        {
            us = USER_NONE;
            if (!strncmp("retweeted_status",content,length))// && ! cur->retweeted )
            {
                cur->retweeted = EINA_TRUE;
                cur->retweeted_status = calloc(1,sizeof(EbirdStatus));
                s = RETWEETED;
            }
            else if (!strncmp("created_at", content, length))// && ! cur->retweeted)
                s = CREATEDAT;
            else if (!strncmp("text",content,length))// && ! cur->retweeted) 
                s = TEXT;
            else if (!strncmp("id",content,length))// && ! cur->retweeted)
                s = ID;
            else if (!strncmp("retweeted",content,length))
                s = RETWEETED;
            else if (!strncmp("user",content,length))// && ! cur->retweeted)
            {
                s = USER;
                cur->user = calloc(1,sizeof(EbirdAccount));
            }
            else if (!strncmp("screen_name",content,length))// && ! cur->retweeted)
            {
                s = USER;
                us = SCREEN_NAME;
            }
            else
                s = NONE;
        }
        else
        {
            if (!strncmp("created_at", content, length))// && ! cur->retweeted)
                rt_s = CREATEDAT;
            else if (!strncmp("text",content,length))// && ! cur->retweeted) 
                rt_s = TEXT;
            else if (!strncmp("id",content,length))// && ! cur->retweeted)
                rt_s = ID;
            else if (!strncmp("retweeted",content,length))
                rt_s = RETWEETED;
            else if (!strncmp("user",content,length))// && ! cur->retweeted)
            {
                rt_s = USER;
                cur->retweeted_status->user = calloc(1,sizeof(EbirdAccount));
            }
            else if (!strncmp("screen_name",content,length))// && ! cur->retweeted)
            {
                rt_s = USER;
                rt_us = SCREEN_NAME;
            }
            else
                rt_s = NONE;

            printf("HoHo\n");
        }
    }
    else if (cur && type == EINA_SIMPLE_XML_DATA)
    {
        char *ptr = strndup(content,length);
//        printf("DEBUG =====> %s [%d]\n",ptr,s);
//        printf("DEBUG ~~~~~> %s [%d]\n",ptr,CREATEDAT);
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
                case RETWEETED:
                    cur->retweeted = EINA_TRUE;
            }
            switch(us)
            {
                case SCREEN_NAME:
                    //                printf("DEBUG ICI [%d][%s]\n",us,ptr);
                    cur->user->username = ptr;
                    break;
                case USER_NONE:
                    break;
            }
        }
        else
        {
            switch(rt_s) 
            {
                case CREATEDAT:
                    cur->retweeted_status->created_at = ptr;
                    break;
                case TEXT:
                    //                printf("===> [DEBUG][%s]\n",ptr);
                    cur->retweeted_status->text = ptr;
                    break;
                case ID:
                    cur->retweeted_status->id = ptr;
                    break;
                case RETWEETED:
                    cur->retweeted_status->retweeted = EINA_TRUE;
            }


            switch(rt_us)
            {
                case SCREEN_NAME:
                    //                printf("DEBUG ICI [%d][%s]\n",us,ptr);
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
        *data = eina_list_append(*data,cur);
        cur = NULL;
    }
    else if (cur && type == EINA_SIMPLE_XML_CLOSE && !strncmp("retweeted_status",content,length))
    {
//        printf("CLOSE\n");
        if (cur->retweeted)
            cur->retweeted = EINA_FALSE;
    }
    return EINA_TRUE;
}

/* END OF TRY  */

Eina_List *
ebird_load_timeline(Eina_Simple_XML_Node_Root *root, Eina_List *list)
{
    Eina_Simple_XML_Node *node;
    Eina_Simple_XML_Node_Tag *tag;
    Eina_Simple_XML_Node_Tag *parent; 
    Eina_Simple_XML_Node_Tag *ancestor; 
    Eina_Simple_XML_Node_Data *data;
    EbirdStatus *status;

    if (list)
        status = (EbirdStatus*)(eina_list_last(list)->data);

    EINA_INLIST_FOREACH(root->children,node)
    {
        tag = (Eina_Simple_XML_Node_Tag*)node;
        parent = tag->base.parent;

        if (node->type == EINA_SIMPLE_XML_NODE_TAG)
        {
            if (!strcmp(tag->name,"status") && !strcmp(parent->name,"statuses"))
            {
                status = calloc(1,sizeof(EbirdStatus));
                status->user = NULL;
                status->retweeted = EINA_FALSE;
                list = eina_list_append(list,status);
            }

            if (!strcmp(tag->name,"created_at") && !strcmp(parent->name,"status"))
                status->created_at = NULL;
            if (!strcmp(tag->name,"id") && !strcmp(parent->name,"status"))
                status->id = NULL;
            if (!strcmp(tag->name,"text") && !strcmp(parent->name,"status"))
                status->text = NULL;

            if (!strcmp(tag->name,"retweeted") && !strcmp(parent->name,"status"))
                status->retweeted = EINA_TRUE;

            if (!strcmp(tag->name,"user") && !strcmp(parent->name,"status"))
            {
                //puts("DEBUG CALLOC USER");
                status->user = calloc(1,sizeof(EbirdAccount));
            }

            if (!strcmp(tag->name,"screen_name") && !strcmp(parent->name,"user"))
            {
                //puts("USER NAME\n");
                if (!status->retweeted)
                    status->user->username = NULL;
            }

            //printf("DEBUG ITERATION\n");
            list = ebird_load_timeline(tag,list);
        }
        if (node->type == EINA_SIMPLE_XML_NODE_DATA)
        {
            Eina_Simple_XML_Node_Data *data = (Eina_Simple_XML_Node_Data *)node;

            if (!status->created_at)
                status->created_at = eina_stringshare_add(data->data);
            else if (!status->id)
                status->id = eina_stringshare_add(data->data);
            else if (!status->text)
                status->text = eina_stringshare_add(data->data);
            else if (!status->retweeted)
                if (!strcmp(data->data,"true"))
                    status->retweeted = EINA_TRUE;
                else
                    status->retweeted = EINA_FALSE;
            else if (status->user)
            {
                if (status->user->username)
                    status->user->username = eina_stringshare_add(data->data);
            }
        }
    }
    return list;
}

Eina_List *
ebird_home_timeline_get(OauthToken *request, EbirdAccount *acc)
{

    char *xml;
    char *timeline_url;
    Eina_List *timeline = NULL;
    Eina_Simple_XML_Node_Root *root;

    timeline_url =  oauth_sign_url2(EBIRD_HOME_TIMELINE_URL,
                                NULL,
                                OA_HMAC,
                                NULL,
                                request->consumer_key,
                                request->consumer_secret,
                                acc->access_token_key,
                                acc->access_token_secret);

    xml = ebird_http_get(timeline_url);
//    printf("DEBUG\n\n\n%s\n\n\n",xml);
    eina_simple_xml_parse(xml,strlen(xml),EINA_TRUE,_parse_timeline, &timeline);
//    root = eina_simple_xml_node_load(xml,strlen(xml)+1,EINA_TRUE);
//    timeline = ebird_load_timeline(root, timeline);
//    eina_simple_xml_node_root_free(root);
    return timeline;
}

char *
ebird_home_timeline_xml_get(OauthToken *request, EbirdAccount *acc)
{

    char *timeline;
    char *timeline_url;

    timeline_url =  oauth_sign_url2(EBIRD_HOME_TIMELINE_URL,
                                NULL,
                                OA_HMAC,
                                NULL,
                                request->consumer_key,
                                request->consumer_secret,
                                acc->access_token_key,
                                acc->access_token_secret);

    timeline = ebird_http_get(timeline_url);
            
    return timeline;
}

char *
ebird_user_show(EbirdAccount *acc)
{
    char buf[EBIRD_URL_MAX];
    char *infos;
    char *url;

    url = strdup(EBIRD_USER_SHOW_URL);

    snprintf(buf,sizeof(buf),"%s&screen_name=%s&userid=%s",
             url,
             acc->username,
             acc->userid);

    infos = ebird_http_get(buf);
    free(url);
    return infos;
}
