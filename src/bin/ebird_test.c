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

    memset(&request_token, 0, sizeof(OauthToken));
    memset(&account, 0, sizeof(EbirdAccount));

    ebird_id_load(&request_token);

    if (ecore_file_exists(EBIRD_ACCOUNT_FILE))
        ebird_account_load(&account);
    else
    {
        account.username = strdup(EBIRD_USER_SCREEN_NAME);
        account.passwd   = strdup(EBIRD_USER_PASSWD);
    }

    //printf("\nDEBUG[main] Step[1][Request Token]\n");
    ebird_token_request_get(&request_token);
    if (request_token.token)
    {

      //  printf("\nDEBUG[main] Step[1][URL][%s]\n", request_token.url);
      //  printf("\nDEBUG[main] Step[1][TOKEN][%s]\n", request_token.token);
      //  printf("\nDEBUG[main] Step[1][TOKEN KEY][%s]\n", request_token.key);
      //  printf("\nDEBUG[main] Step[1][TOKEN SECRET][%s]\n", request_token.secret);
      //  printf("*****************************************\n");
      //  printf("\nDEBUG[main] Step[2][Request Direct Token]\n");

        if (account.access_token_key)
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
            ebird_credentials_verify(&request_token, &account);
            //credentials = ebird_verify_credentials(&request_token, &account);
            printf("%s\n",credentials);

            /*
            puts("HOME TIMELINE");
            puts("=============");
            timeline     = ebird_home_timeline_get(&request_token, &account);
            show_timeline(timeline);
            ebird_timeline_free(timeline);

            
            puts("\nPUBLIC TIMELINE\n");
            pubtimeline  = ebird_public_timeline_get(&request_token, &account);
            show_timeline(pubtimeline);
            ebird_timeline_free(pubtimeline);

            puts("\nUSER TIMELINE\n");
            usertimeline = ebird_user_timeline_get(&request_token, &account);
            show_timeline(usertimeline);
            ebird_timeline_free(usertimeline);

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
        {
            ebird_direct_token_get(&request_token);
            ebird_read_pin_from_stdin(&request_token);
            ebird_authorise_app(&request_token,&account);
            timeline = ebird_home_timeline_get(&request_token, &account);
            printf("%s\n",timeline);

        }

        ecore_main_loop_begin();

        ecore_main_loop_quit();

        ebird_shutdown();

    }
    else
    {
        printf("Error on request token get\n");
        ebird_shutdown();
        return -1;
    }

    return 0;
}
