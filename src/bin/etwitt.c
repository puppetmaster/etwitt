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

typedef struct _Twitt       Twitt;
typedef struct _Account     Account;
typedef struct _Interface   Etwitt_Iface;
typedef struct _ConfigIface Etwitt_Config_Iface;

#define MESSAGES_LIMIT 140

struct _Account
{
   const char *username;
   const char *password;
   const char *realname;
   const char *avatar;
};

struct _Twitt
{
   const char *message;
   const char *date;
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
   Evas_Object         *tw_box;
   Evas_Object         *list;
   Elm_Theme           *theme;
   Etwitt_Config_Iface *config;
   Ebird_Object        *eobj;
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

static char *
_list_item_default_label_get(void        *data,
                             Evas_Object *obj __UNUSED__,
                             const char  *part)
{
   Twitt *twitt = data;
   if (!strcmp(part, "elm.text") && twitt->message)
     return strdup(twitt->message);
   else if (!strcmp(part, "elm.date") && twitt->date)
     return strdup(twitt->date);
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
   Twitt *twitt = data;

   printf("ICON GET !!\n");

   if (!strcmp(part, "elm.swallow.icon"))
     {
        ic = elm_icon_add(obj);
        //elm_icon_file_set(ic, _theme_file_get(), twitt->icon);
        puts("HERE!!!");
        elm_icon_file_set(ic, twitt->icon, NULL);
        evas_object_size_hint_min_set(ic, 32, 32);
        evas_object_show(ic);
     }

   return ic;
}

static Elm_Genlist_Item_Class itc_default = {
   "twitt",
   {
      _list_item_default_label_get,
      _list_item_default_icon_get,
      NULL,
      NULL
   }
};

static void
etwitt_add_twitt(Etwitt_Iface *interface,
                 EbirdStatus  *status)
{
   Twitt *twitt;
   Elm_Genlist_Item *egi;

   char date[PATH_MAX];
   time_t tw_time;
   struct tm *tb;

   tw_time = time(NULL);
   tb = localtime(&tw_time);
   strftime(date, sizeof(date), "%a %d %b %Y %H:%M:%S", tb);

   twitt = calloc(1, sizeof(Twitt));

   twitt->message = eina_stringshare_add(status->text);
   printf("DEBUG twitt date -> [%s]\n",status->created_at);
   twitt->date = eina_stringshare_add(status->created_at);
   twitt->icon = eina_stringshare_add(status->user->avatar);
   printf("DEBUG twitt icon -> [%s]\n", twitt->icon);
   twitt->name = eina_stringshare_add(interface->eobj->account->realname);

   egi = elm_genlist_item_append(interface->list, &itc_default, twitt, NULL,
                                 ELM_GENLIST_ITEM_NONE, NULL, NULL);

   elm_genlist_item_show(egi);
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
_show_configuration(void        *data,
                    Evas_Object *obj __UNUSED__,
                    void        *event_info __UNUSED__)
{
   Etwitt_Iface *iface = data;
   edje_object_signal_emit(elm_layout_edje_get(iface->layout), "SHOW_CONFIG", "code");
   elm_photo_file_set(iface->config->ph_avatar,iface->eobj->account->avatar);
   printf("DEBUG {[%s]}\n",iface->eobj->account->avatar);
   printf("Callback _show_configuration\n");
}

static void
_show_roll(void        *data,
           Evas_Object *obj __UNUSED__,
           void        *event_info __UNUSED__)
{
   Etwitt_Iface *iface = data;

   evas_object_show(iface->list);
   evas_object_show(iface->tw_box);
   edje_object_signal_emit(elm_layout_edje_get(iface->layout), "HIDE_CONFIG", "code");
   ebird_timeline_home_get(iface->eobj, _timeline_get_cb, iface);
   printf("Callback _refresh_roll\n");
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

   printf("Button pressed\n");

   //FIXME etwitt_add_twitt(data, msg);
   elm_entry_entry_set(infos->entry, "");
   free(msg);
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

   elm_object_theme_set(interface->win, interface->theme);
}

static void
etwitt_main_toolbar_add(Etwitt_Iface *interface)
{
   /*
      interface->panel = elm_panel_add(interface->win);
      elm_panel_orient_set(interface->panel, ELM_PANEL_ORIENT_TOP);
      evas_object_size_hint_weight_set(interface->panel, EVAS_HINT_EXPAND, 0);
      evas_object_size_hint_align_set(interface->panel, EVAS_HINT_FILL, EVAS_HINT_FILL);
    */

     interface->toolbar = elm_toolbar_add(interface->win);
     elm_toolbar_mode_shrink_set(interface->toolbar, ELM_TOOLBAR_SHRINK_SCROLL);
     //elm_toolbar_mode_shrink_set(interface->toolbar, ELM_TOOLBAR_SHRINK_NONE);
     elm_toolbar_homogeneous_set(interface->toolbar, 0);
     evas_object_size_hint_weight_set(interface->toolbar, EVAS_HINT_EXPAND, 0);
     evas_object_size_hint_align_set(interface->toolbar, EVAS_HINT_FILL, EVAS_HINT_FILL);
     evas_object_show(interface->toolbar);

     elm_toolbar_item_append(interface->toolbar, "refresh", "TwittRoll", _show_roll, interface);
     elm_toolbar_item_append(interface->toolbar, "folder-new", "Account", _show_configuration, interface);

     //elm_panel_content_set(interface->panel, interface->toolbar);
     //elm_object_part_content_set(interface->layout,"toolbar",interface->panel);
     elm_object_part_content_set(interface->layout, "toolbar", interface->toolbar);
     //evas_object_show(interface->panel);
}

static void
etwitt_twitt_bar_add(Etwitt_Iface *interface)
{
   Evas_Object *icon;
   Evas_Object *button;
   char buf[PATH_MAX];

   interface->tw_box = elm_box_add(interface->win);
   elm_box_horizontal_set(interface->tw_box, EINA_TRUE);
   elm_box_homogeneous_set(interface->tw_box, EINA_FALSE);
   evas_object_size_hint_weight_set(interface->tw_box, EVAS_HINT_EXPAND, 0.0);

   icon = elm_icon_add(interface->win);
   snprintf(buf, sizeof(buf), "data/images/twitt.png");
   elm_icon_file_set(icon, buf, NULL);
   elm_icon_scale_set(icon, 0.5, 0.5);
   evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_HORIZONTAL, 1, 1);
   elm_box_pack_end(interface->tw_box, icon);
   evas_object_show(icon);

   interface->entry = elm_entry_add(interface->win);
   evas_object_size_hint_weight_set(interface->entry, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(interface->entry, EVAS_HINT_FILL, EVAS_HINT_FILL);

   elm_box_pack_end(interface->tw_box, interface->entry);
   elm_entry_scrollable_set(interface->entry, EINA_TRUE);
   evas_object_show(interface->entry);

   button = elm_button_add(interface->win);
   elm_object_text_set(button, "Twitt!");
   elm_box_pack_end(interface->tw_box, button);
   evas_object_smart_callback_add(button, "clicked", _twitt_bt_press, interface);
   evas_object_show(button);

   elm_object_part_content_set(interface->layout, "entry", interface->tw_box);
   evas_object_show(interface->tw_box);
}

static void
etwitt_roll_add(Etwitt_Iface *interface)
{
   interface->list = elm_genlist_add(interface->win);
   elm_genlist_height_for_width_mode_set(interface->list, EINA_TRUE);
   evas_object_show(interface->list);
   elm_genlist_homogeneous_set(interface->list, EINA_FALSE);
   elm_object_part_content_set(interface->layout, "roll", interface->list);
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
   elm_fileselector_button_icon_set(iface->config->bt_avatar, ic);
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

   EINA_LIST_FOREACH(timeline, l, st)
     {
        etwitt_add_twitt(iface, st);
     }
}

void
_session_open_cb(Ebird_Object *obj,
                 void         *data,
                 void         *event)
{
   ebird_timeline_home_get(obj, _timeline_get_cb, data);
}

EAPI_MAIN int
elm_main(int    argc,
         char **argv)
{
   Ebird_Object *eobj;
   Etwitt_Iface *iface;

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
   //iface->eobj->account = calloc(1, sizeof(EbirdAccount));
   iface->config = calloc(1, sizeof(Etwitt_Config_Iface));

   if (ecore_file_exists(EBIRD_ACCOUNT_FILE))
     {
        ebird_account_load(iface->eobj);
        iface->eobj->account->avatar = eina_stringshare_add("avatar.png");
     }
   else
     {
        iface->eobj->account->username = eina_stringshare_add("ePuppetMaster");
        iface->eobj->account->passwd = eina_stringshare_add("QUOI COMMENT OU ...");
        // iface->eobj->account->realname = eina_stringshare_add("Philippe Caseiro");
        // iface->eobj->account->avatar = eina_stringshare_add("avatar.png");
     }

   iface->theme = elm_theme_new();
   elm_theme_extension_add(iface->theme, _theme_file_get());
   elm_theme_overlay_add(iface->theme, _theme_file_get());

   // Main window creation
   etwitt_win_add(iface);

   // Main menu bar
   etwitt_main_toolbar_add(iface);

   etwitt_roll_add(iface);

   etwitt_twitt_bar_add(iface);

   // Configuration
   etwitt_config_iface_add(iface);

   // Opening Session
   ebird_session_open(iface->eobj, _session_open_cb, iface);

   evas_object_resize(iface->win, 460, 540);
   evas_object_show(iface->win);

   //elm_run();
   ecore_main_loop_begin();

   //elm_shutdown();
   ecore_main_loop_quit();

   return 0;
}

ELM_MAIN();
