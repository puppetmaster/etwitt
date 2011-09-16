#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oauth.h>
#include <sys/types.h>
#include <Eet.h>

/*
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Con.h>
*/

#ifndef __UNUSED__
#define __UNUSED__ __attribute__((unused))
#endif

#define EBIRD_URL_MAX 1024
#define EBIRD_PIN_SIZE 12
#define EBIRD_ID_FILE "./id.eet"

#define EBIRD_REQUEST_TOKEN_URL "https://api.twitter.com/oauth/request_token"
#define EBIRD_DIRECT_TOKEN_URL "https://api.twitter.com/oauth/authorize"
#define EBIRD_ACCESS_TOKEN_URL "https://api.twitter.com/oauth/access_token"

#define EBIRD_USER_SCREEN_NAME "xxxxxxx"
#define EBIRD_USER_PASSWD "xxxxxxxx"    //<< percent encode: char "+" => %2B


typedef struct _oauth_token OauthToken;
typedef struct _ebird_account EbirdAccount;

struct _oauth_token
{
    char *consumer_key;
    char *consumer_secret;
    char *url;
    char *token;
    char **token_prm;
    char *key;
    char *secret;
    char *authorisation_url;
    char *authorisation_pin;
    char *authenticity_token;
    char *access_token;
    char **access_token_prm;
    char *access_token_key;
    char *access_token_secret;
    char *callback_confirmed;
    char *userid;
    char *screen_name;
};


struct _ebird_account
{
    char *username;
    char *passwd;
    char *email;
    char *access_token_key;
    char *access_token_secret;
};


/*
struct _request {
         long size;
};


static Eina_Bool
_ebird_url_progress_cb(void *data, int type, void *event)
{

   Ecore_Con_Event_Url_Data *url_data = event;
   if ( url_data->size > 0)
     {
        // append data as it arrives - don't worry where or how it gets stored.
        // Also don't worry about size, expanding, reallocing etc.
        // just keep appending - size is automatically handled.

        eina_strbuf_append_length(data, url_data->data, url_data->size);

        fprintf(stderr, "Appended %d \n", url_data->size);
     }
   return EINA_TRUE;
}

static Eina_Bool
_ebird_url_complete_cb(void *data, int type, void *event)
{
   Ecore_Con_Event_Url_Complete *url_complete = event;
   printf("download completed with status code: %d\n", url_complete->status);

   // get the data back from Eina_Binbuf
   char *ptr = eina_strbuf_string_get(data);
   size_t size = eina_strbuf_length_get(data);

   // process data as required (write to file)
   fprintf(stderr, "Size of data = %d bytes\n", size);
   fprintf(stderr, "String of data = %s bytes\n", ptr);
   //int fd = open("./elm.png", O_CREAT);
   //write(fd, ptr, size);
   //close(fd);

   // free it when done.
   eina_strbuf_free(data);
   free(data);

   ecore_main_loop_quit();

   return EINA_TRUE;
}

char *
ebird_http_get(char *url)
{
    Ecore_Con_Url *ec_url;
    Eina_Strbuf *data;

    ec_url = ecore_con_url_new(NULL);
    ecore_con_url_url_set(ec_url,url);
    ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                            _ebird_url_complete_cb,
                            data);
    ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                            _ebird_url_progress_cb,
                            data);
    ecore_con_url_get(ec_url);

    printf("\nDEBUG DATA [%s]\n",data);
    ecore_con_url_free(ec_url);
    ecore_main_loop_begin();
    return data;

}
*/

static int
ebird_load_id(OauthToken *request_token)
{
    Eet_File *file;
    int size;

    eet_init();

    file = eet_open(EBIRD_ID_FILE,EET_FILE_MODE_READ);
    request_token->consumer_key = strdup(eet_read(file,"key",&size));
    request_token->consumer_secret = strdup(eet_read(file,"secret",&size));
    eet_close(file);

    eet_shutdown();

}



/*
 * name: ebird_error_code_get
 * @param : web script retruned by oauth_http_get()
 * @return : Error code
 */

static int
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
static void
ebird_request_token_get(OauthToken *request)
{
    int res;
    int error_code;

    request->url = oauth_sign_url2(EBIRD_REQUEST_TOKEN_URL, NULL, OA_HMAC,
                                   NULL, request->consumer_key,
                                   request->consumer_secret, NULL, NULL);
    request->token = oauth_http_get(request->url, NULL);
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
                request->key = strdup(&(request->token_prm[0][12]));
                request->secret = strdup(&(request->token_prm[1][19]));
                request->callback_confirmed = strdup(&(request->token_prm[2][25]));
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

static int
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
    request_token->authenticity_token = strdup(key);
    *end = '\'';

    return 0;
}

static int
ebird_authorisation_url_get(OauthToken *request_token)
{
    char buf[EBIRD_URL_MAX];

    snprintf(buf, sizeof(buf),
             EBIRD_DIRECT_TOKEN_URL "?authenticity_token=%s&oauth_token=%s",
             request_token->authenticity_token,
             request_token->key);

    request_token->authorisation_url = strdup(buf);

    return 0;
}

static int
ebird_authorisation_pin_get(OauthToken *request_token,
                            const char *username,
                            const char *userpassword)
{
    char *out_script;
    char *result;
    char url[EBIRD_URL_MAX];
    int retry = 4;
    int i;
    const char *static_header = "Accept:text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\\r\\n\
Accept-Charset:ISO-8859-1,utf-8;q=0.7,*;q=0.7\\r\\n\
Accept-Encoding:gzip, deflate\\r\\n\
Accept-Language:fr,fr-fr;q=0.8,en;q=0.5,en-us;q=0.3\\r\\n\
Connection:keep-alive\\r\\n\
Host:api.twitter.com\\r\\n\
User-Agent:Mozilla/5.0 (X11; Linux x86_64; rv:6.0.2) Gecko/20100101 Firefox/6.0.2\\r\\n";
    char header[EBIRD_URL_MAX];

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


static char *
ebird_access_token_get(OauthToken *request_token,
                       const char *url,
                       const char *con_key,
                       const char *con_secret)
{

   char *acc_url;
   char *acc_token;
   char buf[EBIRD_URL_MAX];
   int res;


   acc_url = oauth_sign_url2(url, NULL, OA_HMAC, NULL,
                             con_key,
                             con_secret,
                             request_token->key,
                             request_token->secret);

   snprintf(buf,sizeof(buf),"%s&oauth_verifier=%s",acc_url,request_token->authorisation_pin);

   acc_token = oauth_http_get(buf,NULL);
   printf("\nDEBUG[ebird_access_token_get][URL][%s]\n",buf);

   if (acc_token)
   {
       printf("\nDEBUG[ebird_access_token_get][RESULT]{%s}\n",acc_token);

       res = oauth_split_url_parameters(request_token->access_token,&request_token->access_token_prm);

       if (res == 4)
       {
           request_token->access_token_key = strdup(&(request_token->access_token_prm[0][12]));
           /*
            *out_access_token_secret = strdup(&(access_token_grant_prm_value[1][19]));
            *out_access_token_uscreen_name = strdup(&(access_token_grant_prm_value[2][12]));
            *out_access_token_user_id = strdup(&(access_token_grant_prm_value[3][8]));
            */
           printf("%s\n",request_token->access_token_key);
       }

   }

   free(acc_url);
   return strdup(buf);
}

static int
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
   script = oauth_http_get(buf, NULL);
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

static int
ebird_auto_authorise_app(OauthToken *request_token, EbirdAccount *account)
{
    if (ebird_authorisation_pin_get(request_token,
                                    account->username,
                                    account->passwd) < 0) 
    {
        return 1;
    }

    request_token->access_token = ebird_access_token_get(request_token,
            EBIRD_ACCESS_TOKEN_URL,
            request_token->consumer_key,
            request_token->consumer_secret);
    account->access_token_key = strdup("xxx");
    account->access_token_secret = strdup("xxx");
    return 0;
}

static int
ebird_authorise_app(OauthToken *request_token, EbirdAccount *account)
{
    char buffer[EBIRD_PIN_SIZE];

    printf("Open this url in a web browser to authorize ebird on access to your account.\n%s\n",
            request_token->authorisation_url);
    printf("Please paste PIN here :\n");
    fgets(buffer,sizeof(buffer),stdin);
    buffer[strlen(buffer)-1] = '\0';
    request_token->authorisation_pin = strdup(buffer);
    printf("You pin is [%s]\n",request_token->authorisation_pin);

    request_token->access_token = ebird_access_token_get(request_token,
            EBIRD_ACCESS_TOKEN_URL,
            request_token->consumer_key,
            request_token->consumer_secret);
    account->access_token_key = strdup(request_token->access_token_key);
    account->access_token_secret = strdup(request_token->access_token_secret);

    return 0;
}


int main(int argc __UNUSED__, char **argv __UNUSED__)
{
    /* Request Token */

    OauthToken request_token;
    EbirdAccount account;


    memset(&request_token, 0, sizeof(OauthToken));
    memset(&account, 0, sizeof(EbirdAccount));
/*
    eina_init();
    ecore_init();
    ecore_con_init();
    ecore_con_url_init();
*/


    ebird_load_id(&request_token);
    account.username = strdup(EBIRD_USER_SCREEN_NAME);
    account.passwd   = strdup(EBIRD_USER_PASSWD);

    printf("\nDEBUG[main] Step[1][Request Token]\n");
    ebird_request_token_get(&request_token);
    if (request_token.token)
    {

        printf("\nDEBUG[main] Step[1][URL][%s]\n", request_token.url);
        printf("\nDEBUG[main] Step[1][TOKEN][%s]\n", request_token.token);
        printf("\nDEBUG[main] Step[1][TOKEN KEY][%s]\n", request_token.key);
        printf("\nDEBUG[main] Step[1][TOKEN SECRET][%s]\n", request_token.secret);
        printf("*****************************************\n");
        printf("\nDEBUG[main] Step[2][Request Direct Token]\n");
        ebird_direct_token_get(&request_token);

        if (ebird_auto_authorise_app(&request_token, &account) == 0)
            return 0;
        else
        {
            if (ebird_authorise_app(&request_token,&account) == 0)
            {
                printf("Manual sucess\n");
                return 0;
            }
            else
                return 255;
        }
    }
    else
    {
        printf("Error on request token get\n");
        printf("\nDEBUG : END\n");

        return 1;
    }
}
