#ifndef EBIRD_H_
#define EBIRD_H_

#define EBIRD_URL_MAX 1024
#define EBIRD_PIN_SIZE 12
#define EBIRD_ID_FILE "./id.eet"
#define EBIRD_ACCOUNT_FILE "./ebird_user.eet"
#define EBIRD_STATUS_MAX 140
#define EBIRD_COOKIE_FILE "./.ebird-cookies"

#define EBIRD_STATUS_URL "https://api.twitter.com/1/statuses/update.xml"
#define EBIRD_PUBLIC_TIMELINE_URL "https://twitter.com/statuses/public_timeline.xml"
#define EBIRD_HOME_TIMELINE_URL "https://api.twitter.com/1/statuses/home_timeline.xml"
#define EBIRD_USER_TIMELINE_URL "https://api.twitter.com/1/statuses/user_timeline.xml"
#define EBIRD_USER_MENTIONS_URL "https://api.twitter.com/1/statuses/mentions.xml"
#define EBIRD_USER_SHOW_URL "http://api.twitter.com/1/users/show.xml"
#define EBIRD_ACCOUNT_CREDENTIALS_URL "https://api.twitter.com/1/account/verify_credentials.xml"

#define EBIRD_REQUEST_TOKEN_URL "https://api.twitter.com/oauth/request_token"
#define EBIRD_DIRECT_TOKEN_URL "https://api.twitter.com/oauth/authorize"
#define EBIRD_ACCESS_TOKEN_URL "https://api.twitter.com/oauth/access_token"

#define EBIRD_USER_SCREEN_NAME "xxxxxxx"
#define EBIRD_USER_PASSWD "xxxxxxxx"    //<< percent encode: char "+" => %2B

#define TAG_STATUS "status>"
#define TAG_USER "user>"

typedef struct _ebird_account EbirdAccount;
typedef struct _ebird_status EbirdStatus;
typedef enum   _state State;
typedef enum   _user_state UserState;
typedef struct _Ebird_Obj Ebird_Object;
typedef struct _oauth_token OauthToken;

typedef void (*Ebird_Session_Cb)(Ebird_Object *obj,void *data);
typedef void (*Ebird_Token_Request_Cb)(Ebird_Object *obj, void *data);
typedef void (*Ebird_Http_Cb)(Ebird_Object *obj);

enum _state
{
    CREATEDAT,
    TEXT,
    ID,
    RETWEETED,
    USER,
    NONE
};

enum _user_state
{
    SCREEN_NAME,
    USER_ID,
    AVATAR,
    USER_NONE
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
  EbirdStatus *retweeted_status;
};


/*
 * API
 *
 */ 

EAPI int ebird_init(void);

EAPI int ebird_shutdown(void);

EAPI Ebird_Object *ebird_add(void);

EAPI void ebird_del(Ebird_Object *obj);


/*
 *
 * Account Operations
 *
 */ 
EAPI Eina_Bool ebird_account_save(Ebird_Object *obj);

EAPI Eina_Bool ebird_account_load(Ebird_Object *obj);

/* 
 *
 * TIMELINES OPERATIONS
 *
 */ 

EAPI void ebird_timeline_home_get(Ebird_Object *obj, Ebird_Session_Cb cb, void *data);

EAPI void ebird_timeline_public_get(Ebird_Object *obj);

EAPI void ebird_timeline_user_get(Ebird_Object *obj);

EAPI void ebird_timeline_mentions_get(Ebird_Object *obj);

EAPI void ebird_timeline_free(Eina_List *timeline);

/*
 *
 * STATUS OPERATIONS
 *
 */ 

EAPI Eina_Bool ebird_status_update(Ebird_Object *obj, char *message);

EAPI Eina_Bool ebird_user_sync(Ebird_Object *obj);

EAPI EbirdAccount *ebird_user_get(char *username, Ebird_Object *obj);

EAPI Eina_Bool ebird_user_show(EbirdAccount *acc);

EAPI char *ebird_credentials_verify(Ebird_Object *obj);


EAPI Eina_Bool ebird_session_open(Ebird_Object *obj,Ebird_Session_Cb cb, void *data);

#endif
