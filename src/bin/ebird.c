#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oauth.h>
#include <sys/types.h>

/*
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Con.h>
*/

#ifndef __UNUSED__
#define __UNUSED__ __attribute__((unused))
#endif

#define EBIRD_URL_MAX 1024

#define EBIRD_REQUEST_TOKEN_URL "https://api.twitter.com/oauth/request_token"
#define EBIRD_DIRECT_TOKEN_URL "https://api.twitter.com/oauth/authorize"
#define EBIRD_ACCESS_TOKEN_URL "https://api.twitter.com/oauth/access_token"

#define EBIRD_USER_SCREEN_NAME "xxxxx"
#define EBIRD_USER_EMAIL "xxxxe@xxxx.com"
#define EBIRD_USER_ID "xxxxxxxx"
#define EBIRD_USER_PASSWD "xxxxxxxx"	//<< percent encode: char "+" => %2B
#define EBIRD_USER_CONSUMER_KEY "xxxxxxxxxxxxxxxxxxxxx"
#define EBIRD_USER_CONSUMER_SECRET "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define EBIRD_USER_ACCESS_TOKEN_KEY "xxxxx"
#define EBIRD_USER_ACCESS_TOKEN_SECRET "xxxx"



typedef struct _oauth_token OauthToken;

struct _oauth_token
{
    char *url;
    char *token;
    char **token_prm;
    char *key;
    char *secret;
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


/*
 * name: ebird_error_code_get
 * @param : web script retruned by oauth_http_get()
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
static void
ebird_request_token_get(OauthToken *request)
{
	int res;
	int error_code;

	request->url = oauth_sign_url2(EBIRD_REQUEST_TOKEN_URL, NULL, OA_HMAC, NULL,
                                   EBIRD_USER_CONSUMER_KEY,
                                   EBIRD_USER_CONSUMER_SECRET, NULL, NULL);
//    request->token = ebird_http_get(request->url);
	request->token = oauth_http_get(request->url,NULL);
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
            res = oauth_split_url_parameters(request->token,&request->token_prm);

            if (res == 3)
            {
                request->key = strdup(&(request->token_prm[0][12]));
                request->secret = strdup(&(request->token_prm[1][19]));
            }
            else
            {
                printf("Error on Request Token [%s]\n",request->token);
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

char *
ebird_authenticity_token_get(char *web_script)
{
	char *keyword;
	char *page;
	char *key;

	keyword = strdup("twttr.form_authenticity_token");

    page = strstr(web_script,keyword);
	if (page)
	{
		strtok(page,"'");
		key = strtok(NULL,"'");
		return key;
	}
	else
		return NULL;

}

char *
ebird_authorisation_get(char *script __UNUSED__,
                            char *authenticity_token,
                            char *username,
                            char *userpassword,
                            char *direct_token_key)
{
    char *auth_url;
    char *auth_params;
    char *out_script;
    char buf[EBIRD_URL_MAX];
    int retry = 4;
    int i;

    char *authenticity_token_label = strdup("authenticity_token");
    char *oauth_token_label = strdup("oauth_token");

    char *username_label = strdup("session%5Busername_or_email%5D");
    char *password_label = strdup("session%5Bpassword%5D");

    auth_url = strdup(EBIRD_DIRECT_TOKEN_URL);

    /*
    printf("\nDEBUG[ebird_authorisation_get] [authenticity_token_label][%i][%s]\n",strlen(authenticity_token_label),authenticity_token_label);
    printf("\nDEBUG[ebird_authorisation_get] [authenticity_token ][%i][%s]\n",strlen(authenticity_token),authenticity_token);
    printf("\nDEBUG[ebird_authorisation_get] [oauth_token_label  ][%i][%s]\n",strlen(direct_token_key),direct_token_key);
    printf("\nDEBUG[ebird_authorisation_get] [username_label     ][%i][%s]\n",strlen(username_label),username_label);
    printf("\nDEBUG[ebird_authorisation_get] [username           ][%i][%s]\n",strlen(username),username);
    printf("\nDEBUG[ebird_authorisation_get] [password_label     ][%i][%s]\n",strlen(password_label),password_label);
    printf("\nDEBUG[ebird_authorisation_get] [userpassword       ][%i][%s]\n",strlen(userpassword),userpassword);
    */


    snprintf(buf,sizeof(buf),"%s=%s&%s=%s&%s=%s&%s=%s&allow=true",
             authenticity_token_label,
             authenticity_token,
             oauth_token_label,
             direct_token_key,
             username_label,
             username,
             password_label,
             userpassword);

    auth_params = strdup(buf);

    for (i = 0 ; i <= retry; i++)
    {
        printf("\nDEBUG[ebird_authorisation_get] TRY[%i]\n",i);
        out_script = oauth_http_get(auth_url,auth_params);
        if (out_script)
        {
            printf("\nDEBUG[ebird_authorisation_get] TRY[%i][SUCCESS]\n* %s?%s\n",i,auth_url,auth_params);

            free(auth_url);
            free(auth_params);
            free(username_label);
            free(authenticity_token_label);
            free(oauth_token_label);
            free(password_label);

            return out_script;

        }
        else
        {
            printf("\nDEBUG[ebird_authorisation_get] TRY[%i][FAILED][%s?%s]\n",i,auth_url,auth_params);
            out_script = NULL;
        }
    }
    return NULL;
}

char *
ebird_authorisation_pin_get(char *webscript __UNUSED__)
{
   char *ret;

   //printf("%s\n",webscript);

   ret = "";
   return ret;
}

char *
ebird_access_token_get(char *url,char *con_key,char *con_secret,OauthToken *request_token, char *pin)
{

   char *acc_url;
   char *acc_token;
   char buf[EBIRD_URL_MAX];


   acc_url = oauth_sign_url2(url, NULL, OA_HMAC, NULL,
                             con_key,
                             con_secret,
                             request_token->key,
                             request_token->secret);

   snprintf(buf,sizeof(buf),"%s&oauth_verifier=%s",acc_url,pin);

   acc_token = oauth_http_get(buf,NULL);
   printf("\nDEBUG[ebird_access_token_get][URL][%s]\n",buf);
   printf("\nDEBUG[ebird_access_token_get][RESULT]{%s}\n",acc_token);
   free(acc_url);

   return strdup(buf);
}

static void
ebird_direct_token_get(OauthToken *request_token)
{

   char *url = NULL;
   char *script = NULL;
   char buf[256];
   char *authenticity_token;
   char *authorisation;
   char *authorisation_pin;
   char *access_token;

   snprintf(buf,sizeof(buf),"%s?oauth_token=%s",EBIRD_DIRECT_TOKEN_URL,request_token->key);

   url = strdup(buf);
   printf("\nDEBUG[ebird_direct_token_get] Step[2.1][Get Authenticity token]\n");
   script = oauth_http_get(url, NULL);
   authenticity_token = ebird_authenticity_token_get(script);
   printf("\nDEBUG[ebird_direct_token_get] Step[2.2][Get Authorisation page\n");
   authorisation = ebird_authorisation_get(script,authenticity_token,EBIRD_USER_SCREEN_NAME,EBIRD_USER_PASSWD,request_token->key);
   authorisation_pin = ebird_authorisation_pin_get(authorisation);
   access_token = ebird_access_token_get(EBIRD_ACCESS_TOKEN_URL,
                                         EBIRD_USER_CONSUMER_KEY,
                                         EBIRD_USER_CONSUMER_SECRET,
                                         request_token,
                                         authorisation_pin);
   free(access_token);
   //free(url);
   //free(authorisation_pin);
}

int main(int argc __UNUSED__, char **argv __UNUSED__)
{
    /* Request Token */

    OauthToken *request_token;
    OauthToken *direct_token;

    request_token = calloc(1,sizeof(OauthToken));
    direct_token  = calloc(1,sizeof(OauthToken));

/*
    eina_init();
    ecore_init();
    ecore_con_init();
    ecore_con_url_init();
*/

    printf("\nDEBUG[main] Step[1][Request Token]\n");
    ebird_request_token_get(request_token);
    if (request_token->token)
    {

        printf("\nDEBUG[main] Step[1][URL][%s]\n",request_token->url);
        printf("\nDEBUG[main] Step[1][TOKEN][%s]\n",request_token->token);
        printf("\nDEBUG[main] Step[1][TOKEN KEY][%s]\n",request_token->key);
        printf("\nDEBUG[main] Step[1][TOKEN SECRET][%s]\n",request_token->secret);
        printf("*****************************************\n");
        printf("\nDEBUG[main] Step[2][Request Direct Token]\n");
        ebird_direct_token_get(request_token);

        free(request_token);
        free(direct_token);
        return 0;
    }
    else
    {
        printf("Error on request token get\n");
        printf("\nDEBUG : END\n");

        free(request_token);
        free(direct_token);

        return 1;
    }
}