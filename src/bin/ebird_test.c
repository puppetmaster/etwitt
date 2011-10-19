#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <oauth.h>

#include <Eina.h>
#include <Ecore.h>
#include <Ecore_File.h>

#include <Ebird.h>

static void
show_timeline(Eina_List *timeline)
{
   Eina_List *l;
   EbirdStatus *st;

   EINA_LIST_FOREACH(timeline, l, st)
     {
        const char *username = NULL;
        const char *username_rt = NULL;
        const char *created_at = NULL;
        const char *text = NULL;

        if (!st)
          continue;

        if (st->retweeted)
          {
             if (st->user)
               username = st->user->username;

             if (st->retweeted_status && st->retweeted_status->user)
               username_rt = st->retweeted_status->user->username;

             if (st->retweeted_status)
               {
                  created_at = st->retweeted_status->created_at;
                  text = st->retweeted_status->text;
               }
             printf("[RT by %s][TW by%s][%s]\n\t%s\n", username,
                    username_rt,
                    created_at,
                    text);
          }
        else
          {
             if (st->user)
               username = st->user->username;

             printf("[TW by %s][%s]\n\t%s\n", username,
                    st->created_at,
                    st->text);
          }
     }
}

static void
_show_credentials(void *data)
{
   char *xml = (char *)data;

   printf("%s\n", xml);
}

static void
_timeline_get_cb(Ebird_Object *obj,
                 void         *data)
{
   Eina_List *timeline = data;
   printf("TIMELINE GET\n");
   show_timeline(timeline);
   puts("============================================");
}

void
_session_opened(Ebird_Object *obj,
                void         *data)
{
   printf("SESSION OPENED DEBUG MESSAGE\n");

   ebird_timeline_home_get(obj, _timeline_get_cb, obj);
   ebird_timeline_public_get(obj, _timeline_get_cb, obj);
}

int
main(int    argc __UNUSED__,
     char **argv __UNUSED__)
{
   char *userinfo;
   char *credentials;
   Ebird_Object *eobj;
   EbirdAccount account;
   Eina_List *timeline;
   Eina_List *pubtimeline;
   Eina_List *usertimeline;
   Eina_List *usermentions;

   if (!ebird_init())
     return -1;

   if (!ecore_file_init())
     {
        ebird_shutdown();
        return -1;
     }

   eobj = ebird_add();

   if (ecore_file_exists(EBIRD_ACCOUNT_FILE))
     ebird_account_load(eobj);
   else
     {
        printf("User account configuration file %s is missing or corrupted\n",
               EBIRD_ACCOUNT_FILE);
     }
/*
    if (ebird_session_open(eobj,_session_opened, NULL))
    {

        printf("User Credentials\n");
        printf("================\n");
        credentials = ebird_credentials_verify(eobj);
        printf("%s\n",credentials);

        puts("HOME TIMELINE");
        puts("=============");
        timeline = ebird_timeline_home_get(eobj);
        show_timeline(timeline);
        ebird_timeline_free(timeline);

        puts("\nPUBLIC TIMELINE\n");
        pubtimeline  = ebird_timeline_public_get(eobj);
        show_timeline(pubtimeline);
        ebird_timeline_free(pubtimeline);

        puts("\nUSER TIMELINE\n");
        usertimeline = ebird_timeline_user_get(eobj);
        show_timeline(usertimeline);
        ebird_timeline_free(usertimeline);

        puts("\nUSER MENTIONS\n");
        usermentions = ebird_timeline_mentions_get(eobj);
        show_timeline(usermentions);
        ebird_timeline_free(usermentions);

        ebird_status_update(eobj, "Ebird Twitt Test #1");

    }
    else
        ebird_shutdown();

 */

   ebird_session_open(eobj, _session_opened, NULL);

   ecore_main_loop_begin();

   ecore_main_loop_quit();

   ebird_del(eobj);
   return 0;
}

