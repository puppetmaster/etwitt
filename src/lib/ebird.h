#ifndef EBIRD_H_
#define EBIRD_H_

#define EBIRD_URL_MAX 1024
#define EBIRD_PIN_SIZE 12
#define EBIRD_ID_FILE "./id.eet"
#define EBIRD_ACCOUNT_FILE "./ebird_user.eet"

#define EBIRD_STATUS_URL "http://api.twitter.com/statuses/update.xml"
#define EBIRD_PUBLIC_TIMELINE_URL "http://twitter.com/statuses/public_timeline.xml"
#define EBIRD_HOME_TIMELINE_URL "http://api.twitter.com/1/statuses/home_timeline.xml"
#define EBIRD_USER_SHOW_URL "https://api.twitter.com/1/users/show.xml"

#define EBIRD_REQUEST_TOKEN_URL "https://api.twitter.com/oauth/request_token"
#define EBIRD_DIRECT_TOKEN_URL "https://api.twitter.com/oauth/authorize"
#define EBIRD_ACCESS_TOKEN_URL "https://api.twitter.com/oauth/access_token"

#define EBIRD_USER_SCREEN_NAME "xxxxxxx"
#define EBIRD_USER_PASSWD "xxxxxxxx"    //<< percent encode: char "+" => %2B

#define TAG_STATUS "status>"
#define TAG_USER "user>"

typedef struct _oauth_token OauthToken;
typedef struct _ebird_account EbirdAccount;
typedef struct _ebird_status EbirdStatus;

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
    char *callback_confirmed;
};


struct _ebird_account
{
    char *username;
    char *passwd;
    char *userid;
    char *access_token_key;
    char *access_token_secret;
    char *avatar;
    char *realname;
};

struct _ebird_status
{
  const char *created_at;
  const char *id;
  const char *text;
  const char *truncated;
  const char *favorited;
  const char *retweet_count;
  Eina_Bool retweeted;
  EbirdAccount *user;
};


Eina_Bool ebird_init();

void ebird_shutdown();

static Eina_Bool _url_data_cb(void *data, int type, void *event_info);

static Eina_Bool _url_complete_cb(void *data, int type, void *event_info);

char *ebird_http_get(char *url);

Eina_Bool ebird_save_account(EbirdAccount *account);

Eina_Bool ebird_load_account(EbirdAccount *account);

int ebird_load_id(OauthToken *request_token);

int ebird_error_code_get(char *string);

void ebird_request_token_get(OauthToken *request);

int ebird_authenticity_token_get(char *web_script, OauthToken *request_token);

int ebird_authorisation_url_get(OauthToken *request_token);

int ebird_authorisation_pin_get(OauthToken *request_token, const char *username, const char *userpassword);

Eina_Bool ebird_authorisation_pin_set(OauthToken *request_token,char *pin);

Eina_Bool ebird_read_pin_from_stdin(OauthToken *request_token);

int ebird_access_token_get(OauthToken *request_token, const char *url, const char *con_key, const char *con_secret, EbirdAccount *account);

int ebird_direct_token_get(OauthToken *request_token);

Eina_Bool ebird_auto_authorise_app(OauthToken *request_token, EbirdAccount *account);

Eina_Bool ebird_authorise_app(OauthToken *request_token, EbirdAccount *account);

Eina_List *ebird_home_timeline_get(OauthToken *request, EbirdAccount *acc);

char *ebird_home_timeline_xml_get(OauthToken *request, EbirdAccount *acc);

char *ebird_user_show(EbirdAccount *acc);

#endif
