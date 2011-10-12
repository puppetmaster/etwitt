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

    EINA_LIST_FOREACH(timeline,l,st)
    {
        //printf("TWITT :[%s][%s]\n",st->created_at,st->text);
        if (st->retweeted)
            printf("[RT by %s][TW by%s][%s]\n\t%s\n",st->user->username,st->retweeted_status->user->username, st->retweeted_status->created_at,st->retweeted_status->text);
        else
            printf("[TW by %s][%s]\n\t%s\n",st->user->username, st->created_at,st->text);

    }
}

static void
_show_credentials(void *data)
{
    char *xml = (char*)data;

    printf("%s\n",xml);
}

int main(int argc __UNUSED__, char **argv __UNUSED__)
{
    char *userinfo;
    char *credentials;
    Ebird_Object *eobj;
    OauthToken request_token;
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
        ebird_account_load(eobj->account);
    else
    {
        account.username = strdup(EBIRD_USER_SCREEN_NAME);
        account.passwd   = strdup(EBIRD_USER_PASSWD);
    }

    //printf("\nDEBUG[main] Step[1][Request Token]\n");

    if (ebird_session_open(eobj->request_token,eobj->account))
    {

        //printf("Account exists !\n");
        /* 
           ebird_user_sync(&account);

           printf("%s\n",account.username);
           printf("%s\n",account.userid);
           printf("%s\n",account.avatar);

        */
        printf("User Credentials\n");
        printf("================\n");
        //ebird_credentials_verify(eobj);
        credentials = ebird_credentials_verify(eobj);
        printf("%s\n",credentials);

        
        puts("HOME TIMELINE");
        puts("=============");
        timeline = ebird_home_timeline_get(eobj);
        show_timeline(timeline);
        ebird_timeline_free(timeline);
        /*
        puts("\nPUBLIC TIMELINE\n");
        pubtimeline  = ebird_public_timeline_get(eobj);
        show_timeline(pubtimeline);
        ebird_timeline_free(pubtimeline);
        */

        puts("\nUSER TIMELINE\n");
        usertimeline = ebird_user_timeline_get(eobj);
        show_timeline(usertimeline);
        ebird_timeline_free(usertimeline);
        /*
           puts("\nUSER MENTIONS\n");
           usermentions = ebird_user_mentions_get(&request_token, &account);
           show_timeline(usermentions);
           ebird_timeline_free(usermentions);

           if (ebird_update_status("JeSuisUnTest",&request_token, &account))
           printf("Twitt OK\n");
           else
           printf("Twitt KO\n");

*/

    }
    else
        ebird_shutdown();

    ebird_del(eobj);
    return 0;
}
