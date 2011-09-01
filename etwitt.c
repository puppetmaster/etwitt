#include <Elementary.h>

typedef struct
{
    Evas_Object *win;
    Evas_Object *roll;
	Evas_Object *entry;

} Twitter;


static void
etwitt_add_twitt(Twitter *info, char* message)
{
    Evas_Object *twitt;
    Evas_Object *twittmsg;


    twitt = elm_bubble_add(info->win);
    elm_object_text_set(twitt,"Twitt");
    elm_object_text_part_set(twitt,"info","20:35 02/02/2012");
    evas_object_size_hint_weight_set(twitt, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(twitt, EVAS_HINT_FILL, EVAS_HINT_FILL);

    twittmsg = elm_anchorblock_add(info->win);
    elm_anchorblock_hover_style_set(twittmsg,"popout");
    elm_anchorblock_hover_parent_set(twittmsg,info->win);
    elm_object_text_set(twittmsg, message);
    elm_bubble_content_set(twitt,twittmsg);

    evas_object_show(twittmsg);
    elm_box_pack_end(info->roll,twitt);
    evas_object_show(twitt);
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
	char *message = elm_entry_entry_get(infos->entry);

    printf("Button pressed\n");

    etwitt_add_twitt(data,message);
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
    Evas_Object *scroll;
    Evas_Object *tw_entry;
    Evas_Object *tw_bt;

    Twitter *infos;
    infos = malloc(sizeof(Twitter));

    Elm_Toolbar_Item *item;

    elm_init(argc, argv);

    win = elm_win_add(NULL,"etwitt", ELM_WIN_BASIC);
    infos->win = win;
    elm_win_title_set(win,"Etwitt");
    evas_object_smart_callback_add(win,"delete,request",_win_del,infos);
    evas_object_resize(win,460,800);
    evas_object_show(win);

    layout = elm_layout_add(win);
    elm_layout_file_set(layout,"phone.edj","elm/layout/roll");
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
    evas_object_size_hint_weight_set(tw_bt, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(tw_bt, EVAS_HINT_FILL, 0.0);
    elm_box_pack_end(tw_box,tw_bt);
    evas_object_smart_callback_add(tw_bt, "clicked", _twitt_bt_press, infos);
    evas_object_show(tw_bt);

    elm_layout_content_set(layout,"entry",tw_box);
    evas_object_show(tw_box);

    scroll = elm_scroller_add(win);
    evas_object_size_hint_weight_set(scroll,EVAS_HINT_EXPAND,EVAS_HINT_EXPAND);
    elm_win_resize_object_add(win,scroll);

    roll = elm_box_add(win);
    infos->roll = roll;
    evas_object_size_hint_weight_set(roll, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(roll, EVAS_HINT_FILL, EVAS_HINT_FILL);
    etwitt_add_twitt(infos,"Message 1");
    etwitt_add_twitt(infos,"Message 2");
    etwitt_add_twitt(infos,"Message 3");
    evas_object_show(roll);


    elm_scroller_content_set(scroll, roll);
    elm_layout_content_set(layout,"roll",scroll);
	evas_object_show(scroll);

    elm_win_resize_object_add (win,layout );
    
    elm_run();
    elm_shutdown();
}
