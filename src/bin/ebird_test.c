#include <ebird.h>

int main(int argc __UNUSED__, char **argv __UNUSED__)
{

    OauthToken request_token;
    EbirdAccount account;
    char *timeline;


    memset(&request_token, 0, sizeof(OauthToken));
    memset(&account, 0, sizeof(EbirdAccount));

    ebird_init();

    ebird_load_id(&request_token);

    if (ecore_file_exists(EBIRD_ACCOUNT_FILE))
        ebird_load_account(&account);
    else
    {
        account.username = strdup(EBIRD_USER_SCREEN_NAME);
        account.passwd   = strdup(EBIRD_USER_PASSWD);
    }

    printf("\nDEBUG[main] Step[1][Request Token]\n");
    ebird_request_token_get(&request_token);
    if (request_token.token)
    {

        printf("\nDEBUG[main] Step[1][URL][%s]\n", request_token.url);
        printf("\nDEBUG[main] Step[1][TOKEN][%s]\n", request_token.token);
        printf("\nDEBUG[main] Step[1][TOKEN KEY][%s]\n", request_token.key);
        printf("\nDEBUG[main] Step[1][TOKEN SECRET][%s]\n", request_token.secret);
        printf("*****************************************\n");
        printf("\nDEBUG[main] Step[2][Request Direct Token]\n");
        ebird_direct_token_get(&request_token);
/*
        if (ebird_auto_authorise_app(&request_token, &account) == 0)
            return 0;
        else
        {
            if (ebird_authorise_app(&request_token,&account) == 0)
            {
                printf("Manual sucess\n");
                return 0;
            }
            else
                return 255;
        }
*/
        if (account.access_token_key)
        {
            printf("Account exists !\n");
            timeline = ebird_home_timeline_get(&request_token, &account);
            printf("%s\n",timeline);

        }
        else
        {
            ebird_read_pin_from_stdin(&request_token);
            ebird_authorise_app(&request_token,&account);
        }
        

        ebird_shutdown();

    }
    else
    {
        printf("Error on request token get\n");
        printf("\nDEBUG : END\n");

        ebird_shutdown();
        return 1;
    }
}
