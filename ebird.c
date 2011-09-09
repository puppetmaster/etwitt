#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oauth.h>
#include <sys/types.h>

#define EBIRD_REQUEST_TOKEN_URL "https://api.twitter.com/oauth/request_token"
#define EBIRD_DIRECT_TOKEN_URL "https://api.twitter.com/oauth/authorize"
#define EBIRD_ACCESS_TOKEN_URL "https://api.twitter.com/oauth/access_token"


#define EBIRD_USER_SCREEN_NAME "xxxxxxxxxxxxxx"
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


/*
 *  FIXME Split into 2 functions
 *
 */
static void 
ebird_request_token_get(OauthToken *request)
{
	int res;
	int i;

	printf("ici\n");
	request->url = oauth_sign_url2(EBIRD_REQUEST_TOKEN_URL, NULL, OA_HMAC, NULL, 
                                   EBIRD_USER_CONSUMER_KEY,                     
                                   EBIRD_USER_CONSUMER_SECRET, NULL, NULL);
	if (request->url)
	{
		request->token = oauth_http_get(request->url,NULL);
		if (strcmp(request->token,"Failed to validate oauth signature and token"))
			printf("Error");
		else
		{
			printf("DEBUG ICI%s\n",request->token);
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
		printf("Error\n");

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

static void
ebird_direct_token_get(char *key)
{

   char *url = NULL;
   char *script = NULL;
   char buf[256];
   char *authenticity_token;

   snprintf(buf,sizeof(buf),"%s?oauth_token=%s",EBIRD_DIRECT_TOKEN_URL,key);

   url = strdup(buf);
   script = oauth_http_get(url, NULL);
   authenticity_token = ebird_authenticity_token_get(script);
   free(url);
}

int main(int argc, char **argv)
{
    /* Request Token */

    OauthToken *request_token;
    OauthToken *direct_token;

    request_token = calloc(1,sizeof(OauthToken));
    direct_token  = calloc(1,sizeof(OauthToken));

    ebird_request_token_get(request_token);
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
