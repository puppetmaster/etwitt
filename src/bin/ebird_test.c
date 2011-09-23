#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <oauth.h>

#include <Eina.h>
#include <Ecore_File.h>

#include <ebird.h>

#define TAG_STATUS "status>"
#define TAG_USER "user>"

typedef struct _ebird_status EbirdStatus;

struct _ebird_status
{
  const char *created_at;
  const char *id;
  const char *text;
  const char *truncated;
  const char *favorited;
  const char *retweet_count;
  const char *retweeted;
  EbirdAccount *user;

};

Eina_List *
ebird_load_timeline(Eina_Simple_XML_Node_Root *root, Eina_List *list)
{
    Eina_Simple_XML_Node *node;
    Eina_Simple_XML_Node_Tag *tag;
    Eina_Simple_XML_Node_Tag *parent; 
    Eina_Simple_XML_Node_Data *data;
    EbirdStatus *status;

    if (list)
        status = (EbirdStatus*)(eina_list_last(list)->data);

    EINA_INLIST_FOREACH(root->children,node)
    {
        tag = (Eina_Simple_XML_Node_Tag*)node;
        parent = tag->base.parent;
        
        if (node->type == EINA_SIMPLE_XML_NODE_TAG)
        {
            if (!strcmp(tag->name,"status") && !strcmp(parent->name,"statuses"))
            {
                status = calloc(1,sizeof(EbirdStatus));
                list = eina_list_append(list,status);
            }

            if (!strcmp(tag->name,"created_at") && !strcmp(parent->name,"status"))
                status->created_at = NULL;
            if (!strcmp(tag->name,"id") && !strcmp(parent->name,"status"))
                status->id = NULL;
            if (!strcmp(tag->name,"text") && !strcmp(parent->name,"status"))
                status->text = NULL;
            if (!strcmp(tag->name,"retweeted") && !strcmp(parent->name,"status"))
                status->retweeted = NULL;

            list = ebird_load_timeline(tag,list);
        }
        if (node->type == EINA_SIMPLE_XML_NODE_DATA)
        {
            Eina_Simple_XML_Node_Data *data = (Eina_Simple_XML_Node_Data *)node;

            if (!status->created_at)
                status->created_at = eina_stringshare_add(data->data);
            else if (!status->id)
                status->id = eina_stringshare_add(data->data);
            else if (!status->text)
                status->text = eina_stringshare_add(data->data);
            else if (!status->retweeted)
                status->retweeted = eina_stringshare_add(data->data);
        }
    }
    return list;
}

static void
ebird_home_timeline_xml_parse(char *xml)
{
//    EbirdStatus *status;
    Eina_List *list = NULL;
    Eina_List *l;
    Eina_Simple_XML_Node_Root *root;
    EbirdStatus *st;


    //eina_simple_xml_parse(xml,strlen(xml)+1,EINA_TRUE,_timeline_xml_cb,status);


    root = eina_simple_xml_node_load(xml,strlen(xml)+1,EINA_TRUE);

    list = ebird_load_timeline(root, list);
    if (list)
        EINA_LIST_FOREACH(list,l,st)
        {
            printf("LIST-DEBUG-[%s][%s]\n",st->created_at,st->text);
        }
    else
        printf("LIST IS EMPTY\n");
    
    eina_simple_xml_node_root_free(root);
    eina_list_free(list);

}

int main(int argc __UNUSED__, char **argv __UNUSED__)
{

    OauthToken request_token;
    EbirdAccount account;
    char *timeline;
    char *userinfo;

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
            printf("Account exists !\n");
            userinfo = ebird_user_show(&account);
            timeline = ebird_home_timeline_get(&request_token, &account);
//            printf("%s\n",timeline);
//            printf("===%s\n===\n",userinfo);

            // ebird_user_xml_parse(userinfo);
            ebird_home_timeline_xml_parse(timeline);

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
        printf("\nDEBUG : END\n");

        ebird_shutdown();
        return -1;
    }

    return 0;
}
