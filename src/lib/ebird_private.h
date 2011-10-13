#ifndef EBIRD_PRIVATE_H_
#define EBIRD_PRIVATE_H_

#include <Ebird.h>

/*
 * variable and macros used for the eina_log module
 */
extern int _ebird_log_dom_global;

/*
 * Macros that are used everywhere
 *
 * the first four macros are the general macros for the lib
 */
#ifdef EBIRD_DEFAULT_LOG_COLOR
# undef EBIRD_DEFAULT_LOG_COLOR
#endif /* ifdef EBIRD_DEFAULT_LOG_COLOR */
#define EBIRD_DEFAULT_LOG_COLOR EINA_COLOR_YELLOW
#ifdef ERR
# undef ERR
#endif /* ifdef ERR */
#define ERR(...)  EINA_LOG_DOM_ERR(_ebird_log_dom_global, __VA_ARGS__)
#ifdef DBG
# undef DBG
#endif /* ifdef DBG */
#define DBG(...)  EINA_LOG_DOM_DBG(_ebird_log_dom_global, __VA_ARGS__)
#ifdef INF
# undef INF
#endif /* ifdef INF */
#define INF(...)  EINA_LOG_DOM_INFO(_ebird_log_dom_global, __VA_ARGS__)
#ifdef WRN
# undef WRN
#endif /* ifdef WRN */
#define WRN(...)  EINA_LOG_DOM_WARN(_ebird_log_dom_global, __VA_ARGS__)
#ifdef CRIT
# undef CRIT
#endif /* ifdef CRIT */
#define CRIT(...) EINA_LOG_DOM_CRIT(_ebird_log_dom_global, __VA_ARGS__)



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

// Move API to async test
struct _ebird_obj
{
    OauthToken *request_token;
    EbirdAccount *account;

    /*
    void (*request_token_get)(Ebird_Object *obj);
    void *request_token_data;

    void (*credentials_verify)(Ebird_Object *obj);
    void *credentials_data;
    */
};

char *ebird_http_get(char *url);
char *ebird_http_post(char *url);
int ebird_error_code_get(char *string);

int ebird_token_authenticity_get(char *web_script, OauthToken *request_token);

int ebird_authorisation_url_get(OauthToken *request_token);

int ebird_authorisation_pin_get(OauthToken *request_token, const char *username, const char *userpassword);

Eina_Bool ebird_authorisation_pin_set(OauthToken *request_token,char *pin);

Eina_Bool ebird_read_pin_from_stdin(OauthToken *request_token);

Eina_Bool ebird_auto_authorise_app(OauthToken *request_token, EbirdAccount *account);

Eina_Bool ebird_authorise_app(OauthToken *request_token, EbirdAccount *account);

int ebird_direct_token_get(OauthToken *request_token);

int ebird_access_token_get(OauthToken *request_token, const char *url, const char *con_key, const char *con_secret, EbirdAccount *account);

void ebird_timeline_free(Eina_List *timeline);

char *ebird_home_timeline_xml_get(OauthToken *request, EbirdAccount *acc);

int ebird_id_load(OauthToken *request_token);

#endif /* EBIRD_PRIVATE_H_ */
