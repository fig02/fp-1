#include <stdlib.h>
#include "menu.h"
#include "settings.h"
#include "commands.h"
#include "resource.h"
#include "fp.h"
#include "watchlist.h"

static uint16_t font_options[] = {
    RES_FONT_FIPPS,
    RES_FONT_NOTALOT35,
    RES_FONT_ORIGAMIMOMMY,
    RES_FONT_PCSENIOR,
    RES_FONT_PIXELINTV,
    RES_FONT_PRESSSTART2P,
    RES_FONT_SMWTEXTNC,
    RES_FONT_WERDNASRETURN,
    RES_FONT_PIXELZIM,
};

static void profile_dec_proc(struct menu_item *item, void *data) {
    fp.profile += SETTINGS_PROFILE_MAX - 1;
    fp.profile %= SETTINGS_PROFILE_MAX;
}

static void profile_inc_proc(struct menu_item *item, void *data) {
    fp.profile += 1;
    fp.profile %= SETTINGS_PROFILE_MAX;
}

static int font_proc(struct menu_item *item, enum menu_callback_reason reason, void *data) {
    if (reason == MENU_CALLBACK_THINK_INACTIVE) {
        if (settings->bits.font_resource != font_options[menu_option_get(item)]) {
            int n_font_options = sizeof(font_options) / sizeof(*font_options);
            for (int i = 0; i < n_font_options; ++i) {
                if (settings->bits.font_resource == font_options[i]) {
                    menu_option_set(item, i);
                    break;
                }
            }
        }
    }
    else if (reason == MENU_CALLBACK_CHANGED) {
        int font_resource = font_options[menu_option_get(item)];
        settings->bits.font_resource = font_resource;
        if (settings->bits.font_resource == RES_FONT_FIPPS) {
            gfx_mode_configure(GFX_MODE_TEXT, GFX_TEXT_NORMAL);
        }
        else {
            gfx_mode_configure(GFX_MODE_TEXT, GFX_TEXT_FAST);
        }
        struct gfx_font *font = resource_get(font_resource);
        menu_set_font(fp.main_menu, font);
        menu_set_cell_width(fp.main_menu, font->char_width + font->letter_spacing);
        menu_set_cell_height(fp.main_menu, font->char_height + font->line_spacing);
        menu_imitate(fp.global, fp.main_menu);
    }
    return 0;
}

static int drop_shadow_proc(struct menu_item *item, enum menu_callback_reason reason, void *data) {
    if (reason == MENU_CALLBACK_CHANGED) {
        settings->bits.drop_shadow = menu_checkbox_get(item);
        gfx_mode_set(GFX_MODE_DROPSHADOW, settings->bits.drop_shadow);
    }
    else if (reason == MENU_CALLBACK_THINK) {
        menu_checkbox_set(item, settings->bits.drop_shadow);
    }
    return 0;
}

static int generic_position_proc(struct menu_item *item, enum menu_callback_reason reason, void *data) {
    int16_t *x = data;
    int16_t *y = x + 1;
    int dist = 2;
    if (input_pad() & BUTTON_Z) {
        dist *= 2;
    }
    switch (reason) {
        case MENU_CALLBACK_ACTIVATE:    input_reserve(BUTTON_Z);  break;
        case MENU_CALLBACK_DEACTIVATE:  input_free(BUTTON_Z);     break;
        case MENU_CALLBACK_NAV_UP:      *y -= dist;               break;
        case MENU_CALLBACK_NAV_DOWN:    *y += dist;               break;
        case MENU_CALLBACK_NAV_LEFT:    *x -= dist;               break;
        case MENU_CALLBACK_NAV_RIGHT:   *x += dist;               break;
        default:
            break;
    }
    return 0;
}

static int menu_position_proc(struct menu_item *item, enum menu_callback_reason reason, void *data) {
    int r = generic_position_proc(item, reason, &settings->menu_x);
    menu_set_pxoffset(fp.main_menu, settings->menu_x);
    menu_set_pyoffset(fp.main_menu, settings->menu_y);
    return r;
}

static int timer_position_proc(struct menu_item *item, enum menu_callback_reason reason, void *data) {
    if (reason == MENU_CALLBACK_ACTIVATE) {
        fp.timer.moving = 1;
    }
    else if (reason == MENU_CALLBACK_DEACTIVATE) {
        fp.timer.moving = 0;
    }
    return generic_position_proc(item, reason, &settings->timer_x);
}

static int input_display_proc(struct menu_item *item, enum menu_callback_reason reason, void *data) {
    if (reason == MENU_CALLBACK_SWITCH_ON) {
        settings->bits.input_display = 1;
    }
    else if (reason == MENU_CALLBACK_SWITCH_OFF) {
        settings->bits.input_display = 0;
    }
    else if (reason == MENU_CALLBACK_THINK) {
        menu_checkbox_set(item, settings->bits.input_display);
    }
    return 0;
}

static int log_proc(struct menu_item *item, enum menu_callback_reason reason, void *data) {
    if (reason == MENU_CALLBACK_SWITCH_ON) {
        settings->bits.log = 1;
    }
    else if (reason == MENU_CALLBACK_SWITCH_OFF) {
        settings->bits.log = 0;
    }
    else if (reason == MENU_CALLBACK_THINK) {
        menu_checkbox_set(item, settings->bits.log);
    }
    return 0;
}

static int log_position_proc(struct menu_item *item, enum menu_callback_reason reason, void *data) {
    fp_log("test log message!");
    return generic_position_proc(item, reason, &settings->log_x);
}

static void activate_command_proc(struct menu_item *item, void *data) {
    int command_index = (int)data;
    if (fp_commands[command_index].proc) {
        fp_commands[command_index].proc();
    }
}

static void tab_prev_proc(struct menu_item *item, void *data) {
    menu_tab_previous(data);
}

static void tab_next_proc(struct menu_item *item, void *data) {
    menu_tab_next(data);
}

static void restore_settings_proc(struct menu_item *item, void *data) {
    settings_load_default();
    apply_menu_settings();
    fp_log("loaded defaults");
}

static void save_settings_proc(struct menu_item *item, void *data) {
    watchlist_store(fp.menu_watchlist);
    settings_save(fp.profile);
    fp_log("saved profile %i", fp.profile);
}

static void load_settings_proc(struct menu_item *item, void *data) {
    if (settings_load(fp.profile)) {
        apply_menu_settings();
        fp_log("loaded profile %i", fp.profile);
    }
    else {
        fp_log("could not load");
    }
}

struct menu *create_settings_menu(void) {
    static struct menu menu;
    static struct menu commands;
    
    /* initialize menu */
    menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_init(&commands, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);


    /*build menu*/
    int y = 0;
    int MENU_X = 16;
    menu.selector = menu_add_submenu(&menu, 0, y++, NULL, "return");
    /* appearance controls */
    menu_add_static(&menu, 0, y, "profile", 0xC0C0C0);
    menu_add_button(&menu, MENU_X, y, "-", profile_dec_proc, NULL);
    menu_add_watch(&menu, MENU_X + 2, y, (uint32_t) & fp.profile, WATCH_TYPE_U8);
    menu_add_button(&menu, MENU_X + 4, y++, "+", profile_inc_proc, NULL);
    menu_add_static(&menu, 0, y, "font", 0xC0C0C0);
    menu_add_option(&menu, 16, y++, "fipps\0""notalot35\0" "origami mommy\0"
                                  "pc senior\0""pixel intv\0""press start 2p\0"
                                  "smw text nc\0""werdna's return\0""pixelzim\0",
                    font_proc, NULL);
    menu_add_static(&menu, 0, y, "drop shadow", 0xC0C0C0);
    menu_add_checkbox(&menu, MENU_X, y++, drop_shadow_proc, NULL);
    menu_add_static(&menu, 0, y, "menu position", 0xC0C0C0);
    menu_add_positioning(&menu, MENU_X, y++, menu_position_proc, NULL);
    menu_add_static(&menu, 0, y, "timer position", 0xC0C0C0);
    menu_add_positioning(&menu, MENU_X, y++, timer_position_proc, NULL);
    menu_add_static(&menu, 0, y, "input display", 0xC0C0C0);
    menu_add_checkbox(&menu, MENU_X, y, input_display_proc, NULL);
    menu_add_positioning(&menu, MENU_X + 2, y++, generic_position_proc, &settings->input_display_x);
    menu_add_static(&menu, 0, y, "log", 0xC0C0C0);
    menu_add_checkbox(&menu, MENU_X, y, log_proc, NULL);
    menu_add_positioning(&menu, MENU_X + 2, y++, log_position_proc, NULL);
    menu_add_submenu(&menu, 0, y++, &commands, "commands");
    /* settings commands */
    menu_add_button(&menu, 0, y++, "save settings", save_settings_proc, NULL);
    menu_add_button(&menu, 0, y++, "load settings", load_settings_proc, NULL);
    menu_add_button(&menu, 0, y++, "restore defaults", restore_settings_proc, NULL);

    /* populate commands menu */
    commands.selector = menu_add_submenu(&commands, 0, 0, NULL, "return");
    const int page_length = 16;
    int n_pages = (COMMAND_MAX + page_length - 1) / page_length;
    struct menu *pages = malloc(sizeof(*pages) * n_pages);
    struct menu_item *tab = menu_add_tab(&commands, 0, 1, pages, n_pages);
    for (int i = 0; i < n_pages; i++) {
        struct menu *page = &pages[i];
        menu_init(page, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
        for (int j = 0; j < page_length; j++) {
            int n = i * page_length + j;
            if (n >= COMMAND_MAX) {
                break;
            }
            if (fp_commands[n].proc) {
                menu_add_button(page, 0, j, fp_commands[n].text, activate_command_proc, (void*)n);
            }
            else {
                menu_add_static(page, 0, j, fp_commands[n].text, 0xC0C0C0);
            }
            binder_create(page, 18, j, n);
        }
    }
    if (n_pages > 0) {
        menu_tab_goto(tab, 0);
    }
    menu_add_button(&commands, 8, 0, "<", tab_prev_proc, tab);
    menu_add_button(&commands, 10, 0, ">", tab_next_proc, tab);
    
    return &menu;
}
