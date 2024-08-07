#pragma once
#include <stdint.h>
#include "msp.h"

typedef enum {
    MSP_DISPLAYPORT_KEEPALIVE,
    MSP_DISPLAYPORT_CLOSE,
    MSP_DISPLAYPORT_CLEAR,
    MSP_DISPLAYPORT_DRAW_STRING,
    MSP_DISPLAYPORT_DRAW_SCREEN,
    MSP_DISPLAYPORT_SET_OPTIONS,
    MSP_DISPLAYPORT_DRAW_SYSTEM
} msp_displayport_cmd_e;

typedef enum {
    MSP_SD_OPTION_30_16,
    MSP_HD_OPTION_50_18,
    MSP_HD_OPTION_30_16,
    MSP_HD_OPTION_60_22
} msp_hd_options_e;

typedef void (*draw_character_func)(uint32_t x, uint32_t y, uint16_t c);
typedef void (*set_options_func)(uint8_t font, msp_hd_options_e is_hd);
typedef void (*clear_screen_func)();
typedef void (*draw_complete_func)();

typedef struct displayport_vtable_s {
    draw_character_func draw_character;
    clear_screen_func clear_screen;
    draw_complete_func draw_complete;
    set_options_func set_options;
} displayport_vtable_t;

int displayport_process_message(displayport_vtable_t *display_driver, msp_msg_t *msg);