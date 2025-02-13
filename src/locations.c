#include <stdint.h>
#include "menu.h"
#include "settings.h"
#include "warp_info.h"
#include "fp.h"

static uint16_t group;
static uint16_t room;
static uint16_t entrance;

static void group_prev_proc(struct menu_item *item, void *data) {
    if (group == 0) {
        group = GROUP_COUNT - 1;
    }
    else {
        group--;
    }

    return;
}

static void group_next_proc(struct menu_item *item, void *data) {
    if (group == GROUP_COUNT - 1) {
        group = 0;
    }
    else {
        group++;
    }

    return;
}

static void room_prev_proc(struct menu_item *item, void *data) {
    if (room == 0) {
        room = GROUPS[group].room_count - 1;
    }
    else {
        room--;
    }

    return;
}

static void room_next_proc(struct menu_item *item, void *data) {
    if (room == GROUPS[group].room_count - 1) {
        room = 0;
    }
    else {
        room++;
    }

    return;
}

static void entrance_prev_proc(struct menu_item *item, void *data) {
    if (entrance == 0) {
        entrance = GROUPS[group].rooms[room].entrance_count - 1;
    }
    else {
        entrance--;
    }

    return;
}

static void entrance_next_proc(struct menu_item *item, void *data) {
    if (entrance == GROUPS[group].rooms[room].entrance_count - 1) {
        entrance = 0;
    }
    else {
        entrance++;
    }

    return;
}

static int warp_info_draw_proc(struct menu_item *item, struct menu_draw_params *draw_params) {
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                               draw_params->alpha));
    struct gfx_font *font = draw_params->font;
    int ch = menu_get_cell_height(item->owner, 1);
    int x = draw_params->x;
    int y = draw_params->y;

    if (room >= GROUPS[group].room_count) {
        room = 0;
    }
    if (entrance >= GROUPS[group].rooms[room].entrance_count) {
        entrance = 0;
    }

    gfx_printf(font, x, y + ch * 0, "%x %s", group, GROUPS[group].group_name);
    gfx_printf(font, x, y + ch * 3, "%x %s", room, GROUPS[group].rooms[room].room_name);
    gfx_printf(font, x, y + ch * 6, "%d/%d", entrance, GROUPS[group].rooms[room].entrance_count);

    return 1;
}

static int current_room_draw_proc(struct menu_item *item, struct menu_draw_params *draw_params) {
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                               draw_params->alpha));
    struct gfx_font *font = draw_params->font;
    int ch = menu_get_cell_height(item->owner, 1);
    int x = draw_params->x;
    int y = draw_params->y;
    gfx_printf(font, x, y + ch * 0, "current room");
    gfx_printf(font, x, y + ch * 1, "g: %x %s", pm_status.group_id, GROUPS[pm_status.group_id].group_name);
    gfx_printf(font, x, y + ch * 2, "r: %x %s", pm_status.room_id, GROUPS[pm_status.group_id].rooms[pm_status.room_id].room_name);
    gfx_printf(font, x, y + ch * 3, "e: %x", pm_status.entrance_id);

    return 1;
}

static void warp_proc() {
    if (fp_warp(group, room, entrance)) {
        fp.saved_group = group;
        fp.saved_room = room;
        fp.saved_entrance = entrance;
    }
}

void create_locations_menu(struct menu *menu) {

    /* initialize menu */
    menu_init(menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu->selector = menu_add_submenu(menu, 0, 0, NULL, "return");

    menu_add_static(menu, 0, 1, "group:", 0xC0C0C0);
    menu_add_button(menu, 0, 2, "<", group_prev_proc, NULL);
    menu_add_button(menu, 1, 2, ">", group_next_proc, NULL);

    menu_add_static(menu, 0, 4, "room:", 0xC0C0C0);
    menu_add_button(menu, 0, 5, "<", room_prev_proc, NULL);
    menu_add_button(menu, 1, 5, ">", room_next_proc, NULL);

    menu_add_static(menu, 0, 7, "entrance:", 0xC0C0C0);
    menu_add_button(menu, 0, 8, "<", entrance_prev_proc, NULL);
    menu_add_button(menu, 1, 8, ">", entrance_next_proc, NULL);

    menu_add_static_custom(menu, 3, 2, warp_info_draw_proc, NULL, 0xC0C0C0);

    menu_add_button(menu, 0, 9, "warp", warp_proc, NULL);

    menu_add_static_custom(menu, 0, 12, current_room_draw_proc, NULL, 0xC0C0C0);
}
