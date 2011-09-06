#include <Elementary.h>

typedef struct _Twitt Twitt;

#define THEME_FILE "./phone.edj"

struct _Twitt
{
    const char *message;
    const char *date;
    const char *name;
    const char *icon;
};

typedef struct
{
   Evas_Object *win;
   Evas_Object *roll;
   Evas_Object *entry;
   Evas_Object *scroller;
   Evas_Object *list;
   char *avatar;

} Twitter;


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

#if 0

static void
etwitt_add_twitt(Twitter *info, char* message)
{
   Evas_Object *twitt;
   Evas_Object *twittmsg;
   Evas_Object *tw_icon;
   char buf[PATH_MAX];


   twitt = elm_bubble_add(info->win);
   //elm_object_text_set(twitt,"Twitt");
   elm_object_text_part_set(twitt,"info","20:35 02/02/2012");
   evas_object_size_hint_weight_set(twitt, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(twitt, EVAS_HINT_FILL, EVAS_HINT_FILL);

   tw_icon = elm_icon_add(info->win);
   snprintf(buf, sizeof(buf), info->avatar);
   elm_icon_file_set(tw_icon, buf, NULL);
   elm_icon_scale_set(tw_icon, 0, 0);
   evas_object_size_hint_aspect_set(tw_icon, EVAS_ASPECT_CONTROL_HORIZONTAL, 1, 1);

   twittmsg = elm_anchorblock_add(info->win);
   elm_anchorblock_hover_style_set(twittmsg,"popout");
   elm_anchorblock_hover_parent_set(twittmsg,info->win);
   elm_bubble_icon_set(twitt, tw_icon);                                                                                                                                                 
   elm_object_text_set(twittmsg, message);
   elm_bubble_content_set(twitt,twittmsg);

   evas_object_show(twittmsg);
   elm_box_pack_end(info->roll,twitt);
   evas_object_show(twitt);
}

#endif

static void
etwitt_add_twitt(Twitter *info, char* message)
{
    Twitt *twitt;
    Elm_Genlist_Item *egi;

    twitt = calloc(1, sizeof(Twitt));

    twitt->message = eina_stringshare_add(message);
    twitt->date = eina_stringshare_add("20:35 02/02/2012");
    twitt->icon = eina_stringshare_add(info->avatar);
    twitt->name = eina_stringshare_add("Nico");
    
    egi = elm_genlist_item_append(info->list, &itc_default, twitt, NULL,
				  ELM_GENLIST_ITEM_NONE, NULL, NULL);
    elm_genlist_item_show(egi);
}

static void
_configuration(void *data, Evas_Object *obj, void *event_info)
{
   printf("Callback _configuration\n");
}

static void
_refresh_roll(void *data, Evas_Object *obj, void *event_info)
{
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
   Twitter *infos = data;
   const char *entry = elm_entry_entry_get(infos->entry);
   char *msg;

   msg = strdup(entry);

   printf("Button pressed\n");

   etwitt_add_twitt(data,msg);
   elm_entry_entry_set(infos->entry,"");
   free(msg);
}

int
main(int argc, char **argv)
{
   Evas_Object *win;
   Evas_Object *layout;
   Evas_Object *edje;
   Evas_Object *roll;
   Evas_Object *toolbar;
   Evas_Object *tw_box;
   /* Evas_Object *scroll; */
   Evas_Object *list;
   Evas_Object *tw_entry;
   Evas_Object *tw_bt;
   Elm_Toolbar_Item *item;
   Elm_Theme *eth;

   Twitter *infos;
   infos = malloc(sizeof(Twitter));

   infos->avatar = "avatar.png";

   elm_init(argc, argv);


   eth = elm_theme_new();
   elm_theme_extension_add(eth, THEME_FILE);
   elm_theme_overlay_add(eth, THEME_FILE);
   

   win = elm_win_add(NULL,"etwitt", ELM_WIN_BASIC);
   infos->win = win;
   elm_win_title_set(win,"Etwitt");
   evas_object_smart_callback_add(win,"delete,request",_win_del,infos);
   elm_object_theme_set(win, eth);

   layout = elm_layout_add(win);
   elm_layout_file_set(layout,THEME_FILE,"elm/layout/roll");
   elm_win_resize_object_add (win,layout);
   evas_object_show(layout);

   toolbar = elm_toolbar_add(win);
   elm_toolbar_mode_shrink_set(toolbar, ELM_TOOLBAR_SHRINK_SCROLL);
   elm_toolbar_item_append(toolbar, "refresh", "Roll",_refresh_roll, layout);
   elm_toolbar_item_append(toolbar, "folder-new", "Accounts",_configuration, layout);
   elm_layout_content_set(layout,"toolbar",toolbar);
   evas_object_show(toolbar);


   tw_box = elm_box_add(win);
   elm_box_horizontal_set(tw_box,EINA_TRUE);
   elm_box_homogeneous_set(tw_box,EINA_TRUE);
   evas_object_size_hint_weight_set(tw_box,EVAS_HINT_EXPAND,0.0);

   tw_entry = elm_entry_add(win);
   infos->entry = tw_entry;
   evas_object_size_hint_weight_set(tw_entry, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(tw_entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(tw_box,tw_entry);
   elm_entry_scrollable_set(tw_entry,EINA_TRUE);
   evas_object_show(tw_entry);

   tw_bt = elm_button_add(win);
   elm_object_text_set(tw_bt,"Twitt!");
//   evas_object_size_hint_weight_set(tw_bt, EVAS_HINT_EXPAND, 0.0);
//   evas_object_size_hint_align_set(tw_bt, EVAS_HINT_FILL, 0.0);
   elm_box_pack_end(tw_box,tw_bt);
   evas_object_smart_callback_add(tw_bt, "clicked", _twitt_bt_press, infos);
   evas_object_show(tw_bt);

   elm_layout_content_set(layout,"entry",tw_box);

   list = elm_genlist_add(win);
   evas_object_show(list);
   elm_layout_content_set(layout,"roll",list);
   infos->list = list;
   
   

   /* scroll = elm_scroller_add(win); */
   /* infos->scroller = scroll; */
   /* evas_object_size_hint_weight_set(scroll,EVAS_HINT_EXPAND,EVAS_HINT_EXPAND); */
   /* roll = elm_box_add(win); */
   /* infos->roll = roll; */
   /* evas_object_size_hint_weight_set(roll, EVAS_HINT_EXPAND, 0.0); */
   /* evas_object_size_hint_align_set(roll, EVAS_HINT_FILL, EVAS_HINT_FILL); */
   /* evas_object_show(roll); */


   /* elm_scroller_content_set(scroll, roll); */
   /* elm_layout_content_set(layout,"roll",scroll); */
/*

   scroll = elm_gengrid_add(win);
   elm_gengrid_item_size_set(scroll,1,10);
   evas_object_size_hint_weight_set(scroll, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_min_set(scroll, 200, 100);
   elm_layout_content_set(layout,"roll",scroll);

*/

   ///evas_object_show(scroll);
   evas_object_resize(win,460,540);
   evas_object_show(win);

   elm_run();
   elm_shutdown();
}
