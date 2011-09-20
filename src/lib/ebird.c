#include <ebird.h>

Eina_Bool
ebird_init()
{
    /* Eina INIT */
    eina_init();

    /* Ecore INIT */
    ecore_init();
    ecore_con_init();
    ecore_con_url_init();
    ecore_file_init();

    /* Eet INIT */
    eet_init();

    return EINA_TRUE;

}

Eina_Bool
ebird_shutdown()
{
    /* ECORE SHUTDOWN */
    ecore_con_url_shutdown();
    ecore_con_shutdown();
    ecore_shutdown();

    /* EINA SHUTDOWN */
    eina_shutdown();

    /* EET SHUTDOWN */
    eet_shutdown();
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

char 
*ebird_http_get(char *url)
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

   printf("DEBUG ebird_save_account\n");

   file = eet_open(EBIRD_ACCOUNT_FILE, EET_FILE_MODE_WRITE);
   printf("ICI\n");

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
    printf("request token: '%s'\n", request->token);
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
                printf("request->key='%s', request->secret='%s',"
                       " request->callback_confirmed='%s'\n",
                       request->key, request->secret,
                       request->callback_confirmed);
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
        printf("\nDEBUG : [%s]\n",request->url);
        printf("Error on Request Token [%s]\n",request->token);
        request->token = NULL;
    }
}

int
ebird_authenticity_token_get(char *web_script, OauthToken *request_token)
{
    char *key,
         *end;

    printf("webscript=%s\n", web_script);
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
        printf("\nDEBUG[ebird_authorisation_get] TRY[%i][%s]\n",i,url);
        printf("%s\n",header);
        out_script = oauth_http_post2(url,NULL, header);
        printf("DEBUG [%s]\n",out_script);
        if (out_script)
        {
            printf("\nDEBUG[ebird_authorisation_get] TRY[%i][SUCCESS]\n* %s\n",i,url);

            printf("========================================================================================================\n");
            printf("%s\n",out_script);
            printf("========================================================================================================\n");
            /*FIXME get the PIN */
            result = strdup("123456789");
            i = retry + 1;
        }
        else
        {
            printf("\nDEBUG[ebird_authorisation_get] TRY[%i][FAILED][%s]\n",i, url);
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
   printf("\nDEBUG[ebird_access_token_get][URL][%s]\n",buf);

   if (acc_token)
   {
       printf("\nDEBUG[ebird_access_token_get][RESULT]{%s}\n",acc_token);
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

   printf("\nDEBUG[ebird_direct_token_get] Step[2.1][Get Authenticity token]\n");
   script = ebird_http_get(buf);
   printf("get '%s'", buf);
   if (ebird_authenticity_token_get(script, request_token) < 0)
       goto error;

   printf("\nDEBUG[ebird_direct_token_get] Step[2.2][Get Authorisation page]\n");

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

    printf("Open this url in a web browser to authorize ebird on access to your account.%s\n",
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
        printf("You pin is [%s]\n",request_token->authorisation_pin);

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

char *
ebird_home_timeline_get(OauthToken *request, EbirdAccount *acc)
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
