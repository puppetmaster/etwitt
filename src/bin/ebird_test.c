#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <oauth.h>

#include <Eina.h>
#include <Ecore_File.h>

#include <ebird.h>


int main(int argc __UNUSED__, char **argv __UNUSED__)
{
    char *userinfo;
    OauthToken request_token;
    EbirdAccount account;
    EbirdStatus *st;
    Eina_List *timeline;
    Eina_List *pubtimeline;
    Eina_List *usertimeline;
    Eina_List *l;

    if (!ebird_init())
        return -1;

    if (!ecore_file_init())
    {
        ebird_shutdown();
        return -1;
    }

    memset(&request_token, 0, sizeof(OauthToken));
    memset(&account, 0, sizeof(EbirdAccount));

    ebird_load_id(&request_token);

    if (ecore_file_exists(EBIRD_ACCOUNT_FILE))
        ebird_load_account(&account);
    else
    {
        account.username = strdup(EBIRD_USER_SCREEN_NAME);
        account.passwd   = strdup(EBIRD_USER_PASSWD);
    }

    //printf("\nDEBUG[main] Step[1][Request Token]\n");
    ebird_request_token_get(&request_token);
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
            userinfo = ebird_user_show(&account);
            timeline = ebird_home_timeline_get(&request_token, &account);
            pubtimeline = ebird_public_timeline_get(&request_token, &account);
            usertimeline = ebird_user_timeline_get(&request_token, &account);

            puts("HOME TIMELINE\n");
            EINA_LIST_FOREACH(timeline,l,st)
            {
                //printf("TWITT :[%s][%s]\n",st->created_at,st->text);
                if (st->retweeted)
                    printf("[RT by %s][TW by%s][%s]\n\t%s\n",st->user->username,st->retweeted_status->user->username, st->retweeted_status->created_at,st->retweeted_status->text);
                else
                    printf("[TW by %s][%s]\n\t%s\n",st->user->username, st->created_at,st->text);

            }

            puts("\nPUBLIC TIMELINE\n");
            EINA_LIST_FOREACH(pubtimeline,l,st)
            {
                //printf("TWITT :[%s][%s]\n",st->created_at,st->text);
                if (st->retweeted)
                    printf("[RT by %s][TW by %s][%s]\n\t%s\n",st->user->username,st->retweeted_status->user->username, st->retweeted_status->created_at,st->retweeted_status->text);
                else
                    printf("[TW by %s][%s]\n\t%s\n",st->user->username, st->created_at,st->text);

            }

            puts("\nUSER TIMELINE\n");
            EINA_LIST_FOREACH(usertimeline,l,st)
            {
                //printf("TWITT :[%s][%s]\n",st->created_at,st->text);
                if (st->retweeted)
                    printf("[RT by %s][TW by %s][%s]\n\t%s\n",st->user->username,st->retweeted_status->user->username, st->retweeted_status->created_at,st->retweeted_status->text);
                else
                    printf("[TW by %s][%s]\n\t%s\n",st->user->username, st->created_at,st->text);
            }
            
            //eina_list_free(timeline);
            ebird_timeline_free(timeline);
            ebird_timeline_free(pubtimeline);
            ebird_timeline_free(usertimeline);


        }
        else
        {
            ebird_direct_token_get(&request_token);
            ebird_read_pin_from_stdin(&request_token);
            ebird_authorise_app(&request_token,&account);
            timeline = ebird_home_timeline_get(&request_token, &account);
            printf("%s\n",timeline);

        }


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
