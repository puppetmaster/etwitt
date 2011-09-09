#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oauth.h>
#include <sys/types.h>

#define EBIRD_REQUEST_TOKEN_URL "https://api.twitter.com/oauth/request_token"
#define EBIRD_DIRECT_TOKEN_URL "https://api.twitter.com/oauth/authorize"
#define EBIRD_ACCESS_TOKEN_URL "https://api.twitter.com/oauth/access_token"


#define EBIRD_USER_SCREEN_NAME "xxxxxxxx"
#define EBIRD_USER_EMAIL "xxxxe@xxxx.com"
#define EBIRD_USER_ID "xxxxx"
#define EBIRD_USER_PASSWD "x-x-x-x-x"	//<< percent encode: char "+" => %2B
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
 *  FIXME Split into 2 functions
 *
 */
static void 
ebird_request_token_get(OauthToken *request)
{
    int res;
    int i;

    request->url = oauth_sign_url2(EBIRD_REQUEST_TOKEN_URL, NULL, OA_HMAC, NULL, 
                                        EBIRD_USER_CONSUMER_KEY,                     
                                        EBIRD_USER_CONSUMER_SECRET, NULL, NULL);
    request->token = oauth_http_get(request->url,NULL);
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

static void
ebird_direct_token_get(OauthToken *request, OauthToken *direct)
{

    char *script;
    int res;

    direct->token = NULL;
    direct->url = NULL;

    direct->url = strdup(EBIRD_DIRECT_TOKEN_URL);
    direct->url = strcat(direct->url,"?oauth_token=");
    direct->url = strcat(direct->url,request->key);

    printf("[DEBUG][%s]\n",direct->url);
    script = oauth_http_get(direct->url, NULL);
    printf("VOILA !\n");

 //   res = oauth_split_url_parameters(direct->token,&direct->token_prm);

}

int main(int argc, char **argv)
{
    /* Request Token */

    OauthToken *request_token;
    OauthToken *direct_token;

    request_token = calloc(1,sizeof(OauthToken));
    direct_token  = calloc(1,sizeof(OauthToken));

    ebird_request_token_get(request_token);
    ebird_direct_token_get(request_token, direct_token);

    printf("*** REQUEST TOKEN ***\n");
    printf("* URL : %s\n",request_token->url);
    printf("* TOKEN : %s\n",request_token->token);
    printf("* TOKEN KEY    : %s\n",request_token->key);
    printf("* TOKEN SECRET : %s\n",request_token->secret);
    printf("**********************\n");

    free(request_token);
    exit(0);
}
