#ifndef EBIRD_PRIVATE_H_
#define EBIRD_PRIVATE_H_

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


char *ebird_http_get(char *url);
char *ebird_http_post(char *url);

#endif /* EBIRD_PRIVATE_H_ */
