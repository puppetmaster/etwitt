#ifndef EBIRD_H_
#define EBIRD_H_

typedef struct _Ebird_Account EbirdAccount;
typedef struct _Ebird_Status EbirdStatus;
typedef enum   _state State;
typedef enum   _user_state UserState;
typedef struct _Ebird_Obj Ebird_Object;
typedef struct _oauth_token OauthToken;

typedef void (*Ebird_Session_Cb)(Ebird_Object *obj,void *data, void *event);
typedef void (*Ebird_Token_Request_Cb)(Ebird_Object *obj, void *data);
typedef void (*Ebird_Http_Cb)(Ebird_Object *obj);

enum _state
{
    CREATEDAT,
    TEXT,
    ID,
    IMAGE,
    REALNAME,
    USERNAME,
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

struct _Ebird_Account
{
    const char *username;
    const char *passwd;
    const char *userid;
    const char *access_token_key;
    const char *access_token_secret;
    const char *avatar;
    const char *realname;
};

struct _Ebird_Status
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

struct _Ebird_Obj
{
   OauthToken *request_token;
   EbirdAccount *account;
   const char *newer_msg_id;
   const char *older_msg_id;
   void           (*cb)(Ebird_Object *obj,
                        void         *data,
                        void         *event);
   void          *data;
   int            fd;
   Ecore_Con_Url *url;
   Eina_Strbuf   *http_data;
   Eina_List     *handlers;
   
   //Eina_List *home_timeline;
   //void (*http_complete_cb)(void *data, int type, void *event_info);
};

EAPI int EBIRD_EVENT_AVATAR_DOWNLOAD = 0;
EAPI int EBIRD_EVENT_PIN_NEED = 0;
EAPI int EBIRD_EVENT_PIN_RECEIVE = 0;
EAPI int EBIRD_EVENT_AUTHORISATION_DONE = 0;

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
 * Authorisation Operations
 * 
 */ 
 
EAPI Eina_Bool ebird_authorisation_pin_set(Ebird_Object *obj, const char *pin);

/* 
 *
 * TIMELINES OPERATIONS
 *
 */ 

EAPI void ebird_timeline_home_get(Ebird_Object *eobj, Ebird_Session_Cb cb, void *data);

EAPI void ebird_timeline_public_get(Ebird_Object *eobj, Ebird_Session_Cb cb, void *data);

EAPI void ebird_timeline_user_get(Ebird_Object *eobj, Ebird_Session_Cb cb, void *data);

EAPI void ebird_timeline_mentions_get(Ebird_Object *eobj, Ebird_Session_Cb cb, void *data);

EAPI void ebird_timeline_free(Eina_List *timeline);

/*
 *
 * STATUS OPERATIONS
 *
 */ 

EAPI void ebird_status_update(char *message, Ebird_Object *obj, Ebird_Session_Cb cb, void *data);

EAPI Eina_Bool ebird_user_sync(Ebird_Object *obj);

EAPI EbirdAccount *ebird_user_get(char *username, Ebird_Object *obj);

EAPI Eina_Bool ebird_user_show(EbirdAccount *acc);

EAPI char *ebird_credentials_verify(Ebird_Object *obj);


EAPI Eina_Bool ebird_session_open(Ebird_Object *obj, Ebird_Session_Cb cb, void *data);
EAPI Eina_Bool ebird_app_authorise(Ebird_Object *obj);

#endif
