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
  Eina_Bool st_status;
  Eina_Bool st_user;
  EbirdAccount *user;

};

static Eina_Bool
_user_xml_cb(void *data, Eina_Simple_XML_Type type, const char *content, unsigned offset, unsigned length)
{
    if (type == EINA_SIMPLE_XML_OPEN)
    {
        printf("XML_OPEN\n");
        printf("%s\n",content);
    }
    else if (type == EINA_SIMPLE_XML_DATA)
    {
        printf("XML_DATA\n");
    }
    return EINA_TRUE;

}

static Eina_Bool
_timeline_xml_attribute_cb(void *data, const char *key, const char *value)
{
    EbirdStatus *st = data;

    printf("DEBUG _timeline_xml_attribute_cb CALLED\n");

    if ( key == "created_at" )
        st->created_at = eina_stringshare_add(value);
    else if ( key == "text" )
        st->text = eina_stringshare_add(value);
    else if ( key == "retweet_count")
        st->retweet_count = eina_stringshare_add(value);
    else if ( key == "retweeted" )
        st->retweeted = eina_stringshare_add(value);
    else if ( key == "user" )
    {
        printf("DEBUG USER\n");
    }
    else
        printf("DEBUG [%s][%s]\n",key,value);

    return EINA_TRUE;

}

static Eina_Bool
_timeline_xml_cb(void *data, Eina_Simple_XML_Type type, const char *content, unsigned offset, unsigned length)
{
    const char *tag;
    EbirdStatus *status = data;

    if (type == EINA_SIMPLE_XML_OPEN)
    {
        if (!strncmp(TAG_STATUS, content, strlen(TAG_STATUS)))
        {
           status->st_status = EINA_TRUE;
        }

        if (!strncmp(TAG_USER, content,strlen(TAG_USER)))
        {
           status->st_user = EINA_TRUE;
        }

    }

    else if (type == EINA_SIMPLE_XML_DATA)
    {
       if (status->st_status)
       {
           tag = eina_simple_xml_tag_attributes_find(content,length);
           printf("TAG[[[%s]]]\n",tag);
           eina_simple_xml_attributes_parse(tag, length - (tag - content),_timeline_xml_attribute_cb,status);
           status->st_status = EINA_FALSE;
       }
       if (status->st_user)
       {
           tag = eina_simple_xml_tag_attributes_find(content,length);
           eina_simple_xml_attributes_parse(tag, length - (tag - content),_timeline_xml_attribute_cb,status);
           status->st_user = EINA_FALSE;
       }
    }
//    printf("\n");
    return EINA_TRUE;

}


static void
ebird_user_xml_parse(char *xml)
{
    eina_simple_xml_parse(xml,strlen(xml)+1, EINA_TRUE ,_user_xml_cb,NULL);
}

static void
ebird_home_timeline_xml_parse(char *xml)
{
    /*
    EbirdStatus *status;

    status = calloc(1,sizeof(EbirdStatus));

    eina_simple_xml_parse(xml,strlen(xml)+1,EINA_TRUE,_timeline_xml_cb,status);
    */

    Eina_Simple_XML_Node_Root *root;
    char *out;

    root = eina_simple_xml_node_load(xml,strlen(xml)+1,EINA_TRUE);
    //out = eina_simple_xml_node_dump(&root->base, " ");
    //puts(out);
    printf("Children   [%i]\n",eina_inlist_count(root->children));
    printf("Attributes [%i]\n",eina_inlist_count(root->attributes));
    printf("[NAME][%s]\n",&root->name);
    free(out);
    eina_simple_xml_node_root_free(root);

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
