#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oauth.h>
#include <sys/types.h>

#define EBIRD_URL_MAX 1024

#define EBIRD_REQUEST_TOKEN_URL "https://api.twitter.com/oauth/request_token"
#define EBIRD_DIRECT_TOKEN_URL "https://api.twitter.com/oauth/authorize"
#define EBIRD_ACCESS_TOKEN_URL "https://api.twitter.com/oauth/access_token"


#define EBIRD_USER_SCREEN_NAME "xxxxxxxxxxxxx"
#define EBIRD_USER_EMAIL "xxxxe@xxxx.com"
#define EBIRD_USER_ID "xxxxxxxxx"
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
 *  FIXME Split into 2 functions
 *
 */
static void 
ebird_request_token_get(OauthToken *request)
{
	int res;
	int i;
	int error_code;

	request->url = oauth_sign_url2(EBIRD_REQUEST_TOKEN_URL, NULL, OA_HMAC, NULL, 
                                   EBIRD_USER_CONSUMER_KEY,                     
                                   EBIRD_USER_CONSUMER_SECRET, NULL, NULL);
	request->token = oauth_http_get(request->url,NULL);
	if (request->token)
    {
        error_code = ebird_error_code_get(request->token);
        if ( error_code != 0)
            printf("Error !\n");
        else
        {
            res = oauth_split_url_parameters(request->token,&request->token_prm);

            if (res = 3)
            {
                request->key = strdup(&(request->token_prm[0][12]));
                request->secret = strdup(&(request->token_prm[1][19]));
            }
            else
            {
                printf("Error on Request Token [%s]",request->token);
            }
        }
    }
    else
        request->token = NULL;

}

char *
ebird_authenticity_token_get(char *web_script)
{
    char *result;
	char *keyword;
	char *page;
	char *key;

	keyword = strdup("twttr.form_authenticity_token");

    page = strstr(web_script,keyword);
	if (page)
	{
		result = strtok(page,"'");
		key = strtok(NULL,"'");
		return key;
	}
	else
		return NULL;

}

char *
ebird_authorisation_get(char *script, 
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

    const char *authenticity_token_label = strdup("authenticity_token");
    const char *oauth_token_label = strdup("oauth_token");

    const char *username_label = strdup("session%5Busername_or_email%5D");
    const char *password_label = strdup("session%5Bpassword%5");

    auth_url = strdup(EBIRD_DIRECT_TOKEN_URL);

    printf("DEBUG\n");
    printf("DEBUG [authenticity_token_label][%i][%s]\n",strlen(authenticity_token_label),authenticity_token_label);
    printf("DEBUG [authenticity_token ][%i][%s]\n",strlen(authenticity_token),authenticity_token);
    printf("DEBUG [oauth_token_label  ][%i][%s]\n",strlen(direct_token_key),direct_token_key);
    printf("DEBUG [username_label     ][%i][%s]\n",strlen(username_label),username_label);
    printf("DEBUG [username           ][%i][%s]\n",strlen(username),username);
    printf("DEBUG [password_label     ][%i][%s]\n",strlen(password_label),password_label);
    printf("DEBUG [userpassword       ][%i][%s]\n",strlen(userpassword),userpassword);
    

    snprintf(buf,sizeof(buf),"%s=%s&%s=%s&%s=%s&%s=%s",
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
        printf("DEBUG : Try [%i]\n",i);
        out_script = oauth_http_get(auth_url,auth_params);
        if (out_script) 
        {
            printf("%s\n%s\n",auth_url,auth_params);

            free(auth_url);
            free(auth_params);
            free(username_label);
            free(authenticity_token_label);
            free(oauth_token_label);
            free(password_label);

            return out_script;
        }
    }
    return NULL;

}

static void
ebird_direct_token_get(char *key)
{

   char *url = NULL;
   char *script = NULL;
   char buf[256];
   char *authenticity_token;
   char *authorisation;

   snprintf(buf,sizeof(buf),"%s?oauth_token=%s",EBIRD_DIRECT_TOKEN_URL,key);

   url = strdup(buf);
   printf("DEBUG : Step 3 Get Authenticity token\n");
   script = oauth_http_get(url, NULL);
   authenticity_token = ebird_authenticity_token_get(script);
   printf("DEBUG : Step 4 Get Authorisation page\n");
   authorisation = ebird_authorisation_get(script,authenticity_token,EBIRD_USER_SCREEN_NAME,EBIRD_USER_PASSWD,key);
   if (authorisation)
       printf("nib\n",authorisation);
   else
       printf("AUTHORISATION_WEB_SCRIPT_IS_NULL\n");
   free(url);
}

int main(int argc, char **argv)
{
    /* Request Token */

    OauthToken *request_token;
    OauthToken *direct_token;

    request_token = calloc(1,sizeof(OauthToken));
    direct_token  = calloc(1,sizeof(OauthToken));

    printf("DEBUG : Step 1 Request TOken\n");
    ebird_request_token_get(request_token);
    if (request_token->token)
    {
        printf("DEBUG : Step 2 Request Direct Token\n");
        ebird_direct_token_get(request_token->key);

        printf("*** REQUEST TOKEN ***\n");
        printf("* URL : %s\n",request_token->url);
        printf("* TOKEN : %s\n",request_token->token);
        printf("* TOKEN KEY    : %s\n",request_token->key);
        printf("* TOKEN SECRET : %s\n",request_token->secret);
        printf("**********************\n");

        free(request_token);
        exit(0);
    }
    else
    {
        printf("Error on request token get\n");
        printf("DEBUG : END");
        exit(1);
    }

}
