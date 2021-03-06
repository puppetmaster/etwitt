#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <time.h>

#include <oauth.h>

#include <Eina.h>
#include <Evas.h>
#include <Ecore_File.h>
#include <Edje.h>
#include <Elementary.h>

#include <Ebird.h>
#include "twitt_date.h"

typedef struct _MicroMessage       MicroMessage;
typedef struct _Account     Account;
typedef struct _Interface   Etwitt_Iface;
typedef struct _ConfigIface Etwitt_Config_Iface;

#define MESSAGES_LIMIT 140

static void etwitt_web_add(Etwitt_Iface *iface, char *url);
static void _show_web(Etwitt_Iface *iface);
static void _show_roll(Etwitt_Iface *iface);

struct _Account
{
   const char *username;
   const char *password;
   const char *realname;
   const char *avatar;
};

struct _MicroMessage
{
   const char *message;
   time_t date;
   const char *name;
   const char *icon;
};

struct _Interface
{
   Evas_Object         *win;
   Evas_Object         *layout;
   Evas_Object         *toolbar;
   Evas_Object         *panel;
   Evas_Object         *roll;
   Evas_Object         *entry;
   Evas_Object         *list;
   Evas_Object         *web;
   Evas_Object         *web_entry;
   Etwitt_Config_Iface *config;
   Ebird_Object        *eobj;
   Elm_Object_Item     *header;
};

struct _ConfigIface
{
   Evas_Object *layout;
   Evas_Object *table;
   Evas_Object *lb_name;
   Evas_Object *en_name;
   Evas_Object *lb_passwd;
   Evas_Object *en_passwd;
   Evas_Object *lb_rename;
   Evas_Object *en_rename;
   Evas_Object *bx_button_bar;
   Evas_Object *bt_save;
   Evas_Object *bt_clear;
   Evas_Object *bt_ok;
   Evas_Object *bx_avatar;
   Evas_Object *bt_avatar;
   Evas_Object *ph_avatar;
};

static const char *_theme_file = NULL;

static void _timeline_get_cb(Ebird_Object *obj,
                             void         *data,
                             void         *event);

static const char *
_theme_file_get(void)
{
   if (!_theme_file)
     {
        char tmp[4096];
        snprintf(tmp, sizeof(tmp), "%s/etwitt/theme/phone.edj", elm_app_data_dir_get());
        _theme_file = eina_stringshare_add(tmp);
        printf("Set theme file to %s\n", _theme_file);
     }

   return _theme_file;
}

static const char *
_get_nice_date(time_t date)
{
    const char *ret;
    int diff, day_diff;
    Eina_Strbuf *strbuf;
    strbuf = eina_strbuf_new();
    time_t now;
    
    time(&now);
    now = mktime(gmtime(&now));
    diff = (int)(now - date);
    day_diff = diff / 86400;

    if (diff < 60)
        eina_strbuf_append_printf(strbuf, "Just now");
    else if (diff < 120)
        eina_strbuf_append_printf(strbuf, "1 minute ago");
    else if (diff < 3600)
        eina_strbuf_append_printf(strbuf, "%d minute ago", diff / 60);
    else if (diff < 7200)
        eina_strbuf_append_printf(strbuf, "1 hour ago");
    else if (diff < 86400)
        eina_strbuf_append_printf(strbuf, "%d hours ago", diff / 3600);
    else if (day_diff == 1)
        eina_strbuf_append_printf(strbuf, "Yesterday");
    else if (day_diff < 7)
        eina_strbuf_append_printf(strbuf, "%d days ago", day_diff);
    else if (day_diff < 31)
        eina_strbuf_append_printf(strbuf, "%d weeks ago", day_diff / 7);

    ret = eina_stringshare_add(eina_strbuf_string_get(strbuf));
    eina_strbuf_free(strbuf);
    return ret;
}

static char *
_list_item_default_label_get(void        *data,
                             Evas_Object *obj __UNUSED__,
                             const char  *part)
{
   MicroMessage *twitt = data;

   if (!strcmp(part, "elm.text") && twitt->message)
     return strdup(twitt->message);
   else if (!strcmp(part, "elm.date") && twitt->date)
     return strdup(_get_nice_date(twitt->date));
   else if (!strcmp(part, "elm.name") && twitt->name)
     return strdup(twitt->name);
   else
     return NULL;
}

static Evas_Object *
_list_item_default_icon_get(void        *data,
                            Evas_Object *obj,
                            const char  *part)
{
   Evas_Object *ic = NULL;
   MicroMessage *twitt = data;

   if (!strcmp(part, "elm.swallow.icon"))
     {
        ic = elm_icon_add(obj);
        //elm_icon_file_set(ic, _theme_file_get(), twitt->icon);
        elm_icon_file_set(ic, twitt->icon, NULL);
        evas_object_size_hint_min_set(ic, 32, 32);
        evas_object_show(ic);
     }

   return ic;
}

static const char *
_markup_add(const char *text)
{
   Eina_Strbuf *strbuf;
   const char *ret;
   char **elts = eina_str_split(text, " ", 0);
   int i;

   strbuf = eina_strbuf_new();

   for (i = 0; elts[i]; i++)
     {
        if (strstr(elts[i], "http://"))
          {
             eina_strbuf_append_printf(strbuf, "<a href=%s><link>%s</link></a> ", elts[i], elts[i]);
          }
        else if (elts[i][0] == '#')
          {
             eina_strbuf_append_printf(strbuf, "<hashtag>%s</hashtag> ", elts[i]);
          }
        else if (elts[i][0] == '@')
          {
             eina_strbuf_append_printf(strbuf, "<username>%s</username> ", elts[i]);
          }
        else
          {
             eina_strbuf_append_printf(strbuf, "%s ", elts[i]);
          }
     }

   ret = eina_stringshare_add(eina_strbuf_string_get(strbuf));
   eina_strbuf_free(strbuf);
   return ret;
}

static Eina_Bool
 _avatar_download_event_cb(void        *data,
                          int           type  __UNUSED__,
                          void         *event __UNUSED__)
{
   Etwitt_Iface *iface = data;

   if (data)
      elm_genlist_realized_items_update(iface->list);
   else
      printf("No list to refresh\n");

   return EINA_TRUE;
}


static Eina_Bool
_pin_need_cb(void *data,
             int  type __UNUSED__,
             void *event)
{
    Etwitt_Iface *iface = data;
    char *url = event;



   printf("DEBUG URL {%s}\n",url);

   etwitt_web_add(iface,url);
   _show_web(iface);

    return EINA_TRUE;

}

static Eina_Bool
_auth_done_cb(void *data,
              int  type __UNUSED__,
              void *event)
{
    Etwitt_Iface *iface = data;
    char *url = event;
   
    _show_roll(iface);

    return EINA_TRUE;

}

static Elm_Genlist_Item_Class itc_default;

Elm_Genlist_Item_Class *_default_item_class_get()
{
   itc_default.item_style = "twitt";
   itc_default.func.text_get = _list_item_default_label_get;
   itc_default.func.content_get = _list_item_default_icon_get;
   itc_default.func.state_get = NULL;
   itc_default.func.del = NULL;

   return &itc_default;
}

static void
etwitt_add_twitt(Etwitt_Iface *interface,
                 EbirdStatus  *status)
{
   MicroMessage *twitt;
   Elm_Object_Item *egi;

   char *date;
   time_t tw_time;
   struct tm *tb;

   tw_time = time(NULL);
   tb = localtime(&tw_time);

   twitt = calloc(1, sizeof(MicroMessage));

   printf("TWITT_ADD[%s]\n",status->text);
   twitt->message = _markup_add(status->text);
   twitt->date = decode_twitt_date(status->created_at);
   twitt->icon = eina_stringshare_add(status->user->avatar);
   twitt->name = eina_stringshare_add(status->user->realname);

   egi = elm_genlist_item_append(interface->list, _default_item_class_get(), twitt, interface->header,
                                 ELM_GENLIST_ITEM_NONE, NULL, NULL);

   elm_genlist_item_show(egi, ELM_GENLIST_ITEM_SCROLLTO_NONE);
}

static void
_file_chosen(void        *data,
             Evas_Object *obj __UNUSED__,
             void        *event_info)
{
   Evas_Object *photo = data;
   const char *file = event_info;
   elm_photo_file_set(photo, file);
   printf("File chosen : %s\n", file);
}

static void
_cfg_clear_bt_cb(void        *data,
                 Evas_Object *obj __UNUSED__,
                 void        *event_info __UNUSED__)
{
   Etwitt_Config_Iface *iface;

   iface = data;

   elm_photo_file_set(iface->ph_avatar, "");
   elm_object_text_set(iface->en_name, "");
   elm_object_text_set(iface->en_rename, "");
   elm_object_text_set(iface->en_passwd, "");
}

static void
_show_configuration(Etwitt_Iface *iface)
{
   elm_object_signal_emit(iface->layout, "show,config", "etwitt");

   elm_photo_file_set(iface->config->ph_avatar,iface->eobj->account->avatar);
}

static void
_show_web(Etwitt_Iface *iface)
{
   puts("DEBUG SINAL show,web EMIT");
   elm_object_signal_emit(iface->layout, "show,web", "etwitt");
}

static void
_show_roll(Etwitt_Iface *iface)
{
   elm_object_signal_emit(iface->layout, "show,timeline", "etwitt");
   elm_object_item_signal_emit(iface->header, "show,loader", "etwitt");
   ebird_timeline_home_get(iface->eobj, _timeline_get_cb, iface);

   printf("Callback _refresh_roll\n");
}

static Eina_Bool
_load_toolbar_timer(void *data)
{
    _show_roll(data);

    return EINA_FALSE;
}

static void
_win_del(void        *data,
         Evas_Object *obj __UNUSED__,
         void        *event_info __UNUSED__)
{
   Etwitt_Iface *iface = data;

   if(iface != NULL)
     {
        free(iface->eobj->account);
        free(iface->config);
        free(iface);
        ebird_shutdown();
     }

   elm_exit();
}

static void
_twitt_bt_press(void        *data,
                Evas_Object *obj __UNUSED__,
                void        *event_info __UNUSED__)
{
   Etwitt_Iface *infos = data;
   const char *entry = elm_entry_entry_get(infos->entry);
   char *msg;

   msg = strdup(entry);

   ebird_status_update(msg, infos->eobj, NULL, NULL);
   elm_entry_entry_set(infos->entry, "");
   free(msg);
}

static void
_toolbar_changed_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
    if (!strcmp(emission, "timeline"))
        _show_roll(data);
    else if (!strcmp(emission, "account"))
        _show_configuration(data);
}

static void
etwitt_win_add(Etwitt_Iface *interface)
{
   interface->win = elm_win_add(NULL, "etwitt", ELM_WIN_BASIC);
   elm_win_title_set(interface->win, "Etwitt");
   evas_object_smart_callback_add(interface->win, "delete,request", _win_del, interface);

   interface->layout = elm_layout_add(interface->win);
   elm_layout_file_set(interface->layout, _theme_file_get(), "elm/layout/roll");
   elm_win_resize_object_add (interface->win, interface->layout);
   evas_object_size_hint_align_set(interface->layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(interface->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(interface->layout);

   edje_object_signal_callback_add(elm_layout_edje_get(interface->layout), 
                            "*", 
                            "toolbar", 
                            _toolbar_changed_cb, interface);
}

static void
_entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
   Etwitt_Iface *interface = data;
   const char *text = elm_entry_markup_to_utf8(elm_object_text_get(interface->entry));
   char char_left[255];
   int nbchar = eina_unicode_utf8_get_len(text);

   if (140 - nbchar < 0)
     snprintf(char_left, sizeof(char_left), "<error>%d left</error>", 140 - nbchar);
   else
     snprintf(char_left, sizeof(char_left), "%d left", 140 - nbchar);
   elm_object_part_text_set(interface->layout, "tweet.left", char_left);
}

static void
etwitt_twitt_bar_add(Etwitt_Iface *interface)
{
   Evas_Object *button;
        
   interface->entry = elm_entry_add(interface->win);
   elm_entry_scrollable_set(interface->entry, EINA_TRUE);
   elm_object_part_content_set(interface->layout, "entry", interface->entry);
   elm_object_style_set(interface->entry, "twitt");
   evas_object_smart_callback_add(interface->entry, "changed", _entry_changed_cb, interface);
   evas_object_show(interface->entry);

   button = elm_button_add(interface->win);
   elm_object_text_set(button, "Send !");
   evas_object_smart_callback_add(button, "clicked", _twitt_bt_press, interface);
   elm_object_part_content_set(interface->layout, "button.tweet", button);
   elm_object_style_set(button, "twitt");
   evas_object_show(button);

   elm_object_part_text_set(interface->layout, "tweet.left", "140 left");
}

static Eina_Bool
_start_loading_anim(void *data)
{
    Etwitt_Iface *interface = data;
    
    elm_object_item_signal_emit(interface->header, "show,loader", "etwitt");

    return EINA_FALSE;
}

static void
_pin_input_bt_cb(void        *data,
                 Evas_Object *obj __UNUSED__,
                 void        *event_info __UNUSED__)
{
   Etwitt_Iface *iface = data;
   const char *pin;
   
   pin = elm_entry_entry_get(iface->web_entry);
   
   ebird_authorisation_pin_set(iface->eobj,pin);
   
}


static void
etwitt_web_add(Etwitt_Iface *iface, char *url)
{   
/*   if (!elm_need_web())
   {
      iface->web = elm_web_add(iface->win);
      evas_object_size_hint_weight_set(iface->web, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
      elm_win_resize_object_add(iface->win, iface->web);
      evas_object_show(iface->web);
      elm_object_part_content_set(iface->layout, "web:label/message", iface->web);
   }
   else*/
   {
      char cmd[2048];
      Evas_Object *lbl, *entry, *box, *bt;

      printf("DEBUG URL : [%s]\n",url);
      snprintf(cmd,sizeof(cmd),"xdg-open '%s'",url);
      printf("DEBUG CMD : [%s]\n",cmd);
      
      printf("Elmentary don't have ewebkit support\n");
      iface->web = elm_win_inwin_add(iface->win);
      elm_object_style_set(iface->web, "default");
      
      box = elm_box_add(iface->win);
      elm_box_horizontal_set(box, EINA_FALSE);
      elm_box_homogeneous_set(box, EINA_FALSE);
      evas_object_show(box);
      
      lbl = elm_label_add(iface->win);
      elm_object_text_set(lbl,"<b>Etwitt</b> is going to open your<br>"
                              "web browser. Please visit the page and<br>"
                              "authorise Ebird to use your account<br>"
                              "and paste PIN here.<br>");
      evas_object_show(lbl);
      
      iface->web_entry = elm_entry_add(iface->win);
      evas_object_size_hint_weight_set(iface->web_entry, EVAS_HINT_FILL, 0.0);
      evas_object_size_hint_align_set(iface->web_entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
      elm_entry_scrollable_set(iface->web_entry, EINA_FALSE);
      //elm_entry_single_line_set(iface->config->en_name, EINA_TRUE);
      elm_object_style_set(iface->web_entry, "twitt");
      elm_entry_single_line_set(iface->web_entry, EINA_TRUE);
      evas_object_show(iface->web_entry);      

      bt = elm_button_add(iface->win);
      evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.5);
      evas_object_size_hint_align_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
      evas_object_show(bt);
      elm_object_style_set(bt, "twitt");
      evas_object_smart_callback_add(bt, "clicked", _pin_input_bt_cb, iface);
      elm_object_text_set(bt, "Ok");
      
      elm_box_pack_end(box,lbl);
      elm_box_pack_end(box,iface->web_entry);
      elm_box_pack_end(box,bt);
      
      elm_win_inwin_content_set(iface->web,box);
      evas_object_show(iface->web);
      ecore_exe_run(cmd,NULL);
      elm_object_part_content_set(iface->layout,"web:label/message",iface->web);
   }
}

static Elm_Genlist_Item_Class itc_timeline_header;

Elm_Genlist_Item_Class *_get_header_item_class()
{
   itc_timeline_header.item_style = "twitt_header";
   itc_timeline_header.func.text_get = NULL;
   itc_timeline_header.func.content_get = NULL;
   itc_timeline_header.func.state_get = NULL;
   itc_timeline_header.func.del = NULL;
   return &itc_timeline_header;
}

static void
etwitt_roll_add(Etwitt_Iface *interface)
{
   interface->list = elm_genlist_add(interface->win);
   elm_genlist_mode_set(interface->list, ELM_LIST_COMPRESS);
   evas_object_show(interface->list);
   elm_genlist_scroller_policy_set(interface->list, ELM_SCROLLER_POLICY_OFF,
                                   ELM_SCROLLER_POLICY_ON);
   elm_genlist_homogeneous_set(interface->list, EINA_FALSE);
   elm_object_part_content_set(interface->layout, "roll", interface->list);
   elm_object_style_set(interface->list, "etwitt");

   
   interface->header = elm_genlist_item_append(interface->list, _get_header_item_class(), NULL, NULL,
                                 ELM_GENLIST_ITEM_GROUP, NULL, NULL);

   ecore_timer_add(0.5, _start_loading_anim, interface);
}

static void
etwitt_config_iface_add(Etwitt_Iface *iface)
{
   Evas_Object *ic;

   iface->config->lb_name = elm_label_add(iface->win);
   elm_object_text_set(iface->config->lb_name, "Username :");
   evas_object_show(iface->config->lb_name);
   elm_object_part_content_set(iface->layout, "config:label/name", iface->config->lb_name);

   iface->config->en_name = elm_entry_add(iface->win);
   evas_object_size_hint_weight_set(iface->config->en_name, EVAS_HINT_FILL, 0.0);
   evas_object_size_hint_align_set(iface->config->en_name, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_entry_scrollable_set(iface->config->en_name, EINA_FALSE);
   elm_entry_single_line_set(iface->config->en_name, EINA_TRUE);
   elm_object_part_content_set(iface->layout, "config:entry/name", iface->config->en_name);
   if (iface->eobj->account->username)
     elm_object_text_set(iface->config->en_name, iface->eobj->account->username);
   evas_object_show(iface->config->en_name);

   iface->config->lb_passwd = elm_label_add(iface->win);
   elm_object_text_set(iface->config->lb_passwd, "Password :");
   evas_object_show(iface->config->lb_passwd);
   elm_object_part_content_set(iface->layout, "config:label/password", iface->config->lb_passwd);

   iface->config->en_passwd = elm_entry_add(iface->win);
   evas_object_size_hint_weight_set(iface->config->en_passwd, EVAS_HINT_FILL, 0.0);
   evas_object_size_hint_align_set(iface->config->en_passwd, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_entry_password_set(iface->config->en_passwd, EINA_TRUE);
   elm_object_part_content_set(iface->layout, "config:entry/password", iface->config->en_passwd);
   evas_object_show(iface->config->en_passwd);

   iface->config->lb_rename = elm_label_add(iface->win);
   elm_object_text_set(iface->config->lb_rename, "Real name :");
   evas_object_show(iface->config->lb_rename);
   elm_object_part_content_set(iface->layout, "config:label/realname", iface->config->lb_rename);

   iface->config->en_rename = elm_entry_add(iface->win);
   evas_object_size_hint_weight_set(iface->config->en_rename, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(iface->config->en_rename, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_entry_scrollable_set(iface->config->en_rename, EINA_FALSE);
   elm_entry_single_line_set(iface->config->en_rename, EINA_TRUE);
   elm_object_part_content_set(iface->layout, "config:entry/realname", iface->config->en_rename);
   evas_object_show(iface->config->en_rename);

   iface->config->bx_avatar = elm_box_add(iface->win);
   elm_box_horizontal_set(iface->config->bx_avatar, EINA_FALSE);
   elm_box_homogeneous_set(iface->config->bx_avatar, EINA_FALSE);
   elm_object_part_content_set(iface->layout, "config:avatarselector", iface->config->bx_avatar);
   evas_object_show(iface->config->bx_avatar);

   ic = elm_icon_add(iface->win);
   elm_icon_standard_set(ic, "file");
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

   iface->config->ph_avatar = elm_photo_add(iface->win);
   elm_photo_fill_inside_set(iface->config->ph_avatar, EINA_TRUE);
   elm_photo_size_set(iface->config->ph_avatar, 100);
//    elm_object_style_set(iface->config->ph_avatar, "shadow");
   elm_box_pack_end(iface->config->bx_avatar, iface->config->ph_avatar);
   evas_object_show(iface->config->ph_avatar);

   iface->config->bt_avatar = elm_fileselector_button_add(iface->win);
   elm_fileselector_button_path_set(iface->config->bt_avatar, "/home");
   elm_object_text_set(iface->config->bt_avatar, "Select a file");
//   elm_fileselector_button_icon_set(iface->config->bt_avatar, ic);
   elm_fileselector_button_inwin_mode_set(iface->config->bt_avatar, EINA_FALSE);
   evas_object_smart_callback_add(iface->config->bt_avatar, "file,chosen", _file_chosen, iface->config->ph_avatar);
   evas_object_show(iface->config->bt_avatar);
   elm_box_pack_end(iface->config->bx_avatar, iface->config->bt_avatar);

   iface->config->bx_button_bar = elm_box_add(iface->win);
   elm_box_horizontal_set(iface->config->bx_button_bar, EINA_TRUE);
   elm_box_homogeneous_set(iface->config->bx_button_bar, EINA_TRUE);
   elm_object_part_content_set(iface->layout, "config:actionbar", iface->config->bx_button_bar);

   iface->config->bt_save = elm_button_add(iface->win);
   evas_object_size_hint_weight_set(iface->config->bt_save, EVAS_HINT_EXPAND, 0.5);
   evas_object_size_hint_align_set(iface->config->bt_save, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(iface->config->bt_save, "Save");
   elm_box_pack_end(iface->config->bx_button_bar, iface->config->bt_save);

   //#FIXME event !!!

   evas_object_show(iface->config->bt_save);

   iface->config->bt_clear = elm_button_add(iface->win);
   elm_object_text_set(iface->config->bt_clear, "Clear");
   evas_object_size_hint_weight_set(iface->config->bt_clear, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(iface->config->bt_clear, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(iface->config->bt_clear, "clicked", _cfg_clear_bt_cb, iface->config);
   elm_box_pack_end(iface->config->bx_button_bar, iface->config->bt_clear);
   evas_object_show(iface->config->bt_clear);
}

static void
_timeline_get_cb(Ebird_Object *obj,
                 void         *data,
                 void         *event)
{
   Eina_List *timeline = event;
   Eina_List *l;
   Etwitt_Iface *iface = data;
   EbirdStatus *st;

   EINA_LIST_REVERSE_FOREACH(timeline, l, st)
     {
        etwitt_add_twitt(iface, st);
     }

   elm_object_item_signal_emit(iface->header, "hide,loader", "etwitt");
}

void
_session_open_cb(Ebird_Object *obj,
                 void         *data,
                 void         *event)
{
    ecore_timer_add(0.5, _load_toolbar_timer, data);
}

EAPI_MAIN int
elm_main(int    argc,
         char **argv)
{
   Ebird_Object *eobj;
   Etwitt_Iface *iface;
   Ecore_Event_Handler *avatar_hdl;
   Ecore_Event_Handler *pin_hdl;
   Ecore_Event_Handler *auth_done;

   if (!ebird_init())
     return -1;

   if (!ecore_file_init())
     {
        ebird_shutdown();
        return -1;
     }

   /* tell elm about our app so it can figure out where to get files */
   printf(" %s %s\n", PACKAGE_BIN_DIR, PACKAGE_DATA_DIR);
   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
   elm_app_info_set(elm_main, "etwitt", "images/logo.png");

   iface = calloc(1, sizeof(Etwitt_Iface));
   iface->eobj = ebird_add();
   iface->config = calloc(1, sizeof(Etwitt_Config_Iface));

   elm_theme_extension_add(NULL, _theme_file_get());

   // Main window creation
   etwitt_win_add(iface);

   etwitt_roll_add(iface);

   etwitt_twitt_bar_add(iface);

   // Configuration
   etwitt_config_iface_add(iface);

   //FIXME HAVE TO BE FREE !!
   avatar_hdl = ecore_event_handler_add(EBIRD_EVENT_AVATAR_DOWNLOAD,
                                    _avatar_download_event_cb, iface);

   pin_hdl = ecore_event_handler_add(EBIRD_EVENT_PIN_NEED,
                                     _pin_need_cb, iface);
                                     
   auth_done = ecore_event_handler_add(EBIRD_EVENT_AUTHORISATION_DONE,
                                       _auth_done_cb, iface);  
                                       
  // eobj->handlers = eina_list_append(eobj->handlers, hdl);

   if (ebird_account_load(iface->eobj))
      iface->eobj->account->avatar = eina_stringshare_add("avatar.png");

   ebird_session_open(iface->eobj, _session_open_cb, iface);

   evas_object_resize(iface->win, 400, 600);

   evas_object_show(iface->win);

   elm_run();

   elm_shutdown();
   
   ebird_shutdown();

   return 0;
}

ELM_MAIN();
