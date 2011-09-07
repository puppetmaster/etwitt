#include <Elementary.h>

typedef struct _Twitt Twitt;
typedef struct _Account Account;
typedef struct _Interface Etwitt_Iface;
#define THEME_FILE "./phone.edj"

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
   Evas_Object *win;
   Evas_Object *layout;
   Evas_Object *toolbar;
   Evas_Object *panel;
   Evas_Object *roll;
   Evas_Object *cfg_bx;
   Evas_Object *entry;
   Evas_Object *tw_box;
   Evas_Object *list;
   Elm_Theme *theme;
   char *avatar;

};


static char *
_list_item_default_label_get(void *data, Evas_Object *obj, const char *part)
{
    Twitt *twitt = data;
    if (!strcmp(part, "elm.text"))
      return strdup(twitt->message);
    else if (!strcmp(part, "elm.date"))
      return strdup(twitt->date);
    else if (!strcmp(part, "elm.name"))
      return strdup(twitt->name);
    else
      return NULL;
}

static Evas_Object *
_list_item_default_icon_get(void *data, Evas_Object *obj, const char *part)
{
    Evas_Object *ic = NULL;
    Twitt *twitt = data;

    if (!strcmp(part, "elm.swallow.icon"))
      {
	 ic = elm_icon_add(obj);
	 elm_icon_file_set(ic, THEME_FILE, twitt->icon);
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
etwitt_add_twitt(Etwitt_Iface *interface, char* message)
{
    Twitt *twitt;
    Elm_Genlist_Item *egi;

    twitt = calloc(1, sizeof(Twitt));

    twitt->message = eina_stringshare_add(message);
    twitt->date = eina_stringshare_add("20:35 02/02/2012");
    twitt->icon = eina_stringshare_add(interface->avatar);
    twitt->name = eina_stringshare_add("Nico");
    
    egi = elm_genlist_item_append(interface->list, &itc_default, twitt, NULL,
				  ELM_GENLIST_ITEM_NONE, NULL, NULL);
    elm_genlist_item_show(egi);
}

static void
_show_configuration(void *data, Evas_Object *obj, void *event_info)
{
   Etwitt_Iface *iface = data;
   evas_object_hide(iface->list);
   evas_object_hide(iface->tw_box);
   printf("Callback _show_configuration\n");
}

static void
_show_roll(void *data, Evas_Object *obj, void *event_info)
{
    Etwitt_Iface *iface = data;
    evas_object_show(iface->list);
    evas_object_show(iface->tw_box);
   printf("Callback _refresh_roll\n");
}

static void
_win_del(void *data, Evas_Object *obj, void *event_info)
{
   if(data != NULL)
     {
	free(data);
     }

   elm_exit();
}

static void
_twitt_bt_press(void *data, Evas_Object *obj,void *event_info)
{
   Etwitt_Iface *infos = data;
   const char *entry = elm_entry_entry_get(infos->entry);
   char *msg;

   msg = strdup(entry);

   printf("Button pressed\n");

   etwitt_add_twitt(data,msg);
   elm_entry_entry_set(infos->entry,"");
   free(msg);
}

static void
etwitt_win_add(Etwitt_Iface *interface)
{
   interface->win = elm_win_add(NULL,"etwitt", ELM_WIN_BASIC);
   elm_win_title_set(interface->win,"Etwitt");
   evas_object_smart_callback_add(interface->win,"delete,request",_win_del,interface);

   interface->layout = elm_layout_add(interface->win);
   elm_layout_file_set(interface->layout,THEME_FILE,"elm/layout/roll");
   elm_win_resize_object_add (interface->win,interface->layout);
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

   elm_toolbar_item_append(interface->toolbar, "refresh", "Roll",_show_roll, interface);
   elm_toolbar_item_append(interface->toolbar, "folder-new", "Accounts",_show_configuration, interface);

   //elm_panel_content_set(interface->panel, interface->toolbar);
   //elm_layout_content_set(interface->layout,"toolbar",interface->panel);
   elm_layout_content_set(interface->layout,"toolbar",interface->toolbar);
   //evas_object_show(interface->panel);

}


static void 
etwitt_twitt_bar_add(Etwitt_Iface *interface)
{
   Evas_Object *box;
   Evas_Object *icon;
   Evas_Object *entry;
   Evas_Object *button;
   char buf[PATH_MAX];

   interface->tw_box = elm_box_add(interface->win);
   elm_box_horizontal_set(interface->tw_box,EINA_TRUE);
   elm_box_homogeneous_set(interface->tw_box,EINA_FALSE);
   evas_object_size_hint_weight_set(interface->tw_box,EVAS_HINT_EXPAND,0.0);

   icon = elm_icon_add(interface->win);
   snprintf(buf, sizeof(buf), "twitt.png");
   elm_icon_file_set(icon, buf, NULL);
   elm_icon_scale_set(icon, 0.5, 0.5);
   evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_HORIZONTAL, 1, 1);
   elm_box_pack_end(interface->tw_box,icon);
   evas_object_show(icon);

   interface->entry = elm_entry_add(interface->win);
   evas_object_size_hint_weight_set(interface->entry, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(interface->entry, EVAS_HINT_FILL, EVAS_HINT_FILL);

   elm_box_pack_end(interface->tw_box,interface->entry);
   elm_entry_scrollable_set(interface->entry,EINA_TRUE);
   evas_object_show(interface->entry);

   button = elm_button_add(interface->win);
   elm_object_text_set(button,"Twitt!");
   elm_box_pack_end(interface->tw_box,button);
   evas_object_smart_callback_add(button, "clicked", _twitt_bt_press, interface);
   evas_object_show(button);

   elm_layout_content_set(interface->layout,"entry",interface->tw_box);
   evas_object_show(interface->tw_box);
}

static void
etwitt_roll_add(Etwitt_Iface *interface)
{

   interface->list = elm_genlist_add(interface->win);
   evas_object_show(interface->list);
   elm_layout_content_set(interface->layout,"roll",interface->list);
}

int
main(int argc, char **argv)
{
   Etwitt_Iface *infos;
   infos = calloc(1,sizeof(Etwitt_Iface));

   infos->avatar = "avatar.png";

   elm_init(argc, argv);

   infos->theme = elm_theme_new();
   elm_theme_extension_add(infos->theme, THEME_FILE);
   elm_theme_overlay_add(infos->theme, THEME_FILE);
   
   // Main window creation
   etwitt_win_add(infos);

   // Main menu bar
   etwitt_main_toolbar_add(infos);

   etwitt_twitt_bar_add(infos);

   etwitt_roll_add(infos);

   
   evas_object_resize(infos->win,460,540);
   evas_object_show(infos->win);

   elm_run();
   elm_shutdown();
}
