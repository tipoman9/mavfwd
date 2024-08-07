#include "msp.h"
#include "msp_displayport.h"


void clear_screen() {
    printf("\033[2J");
}

// Function to move the cursor to a specific position
void move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

// Function to draw a character at a specific position
void draw_character(int row, int col, char ch) {
    move_cursor(row, col);
    printf("%c", ch);
}



static void process_draw_string(displayport_vtable_t *display_driver, uint8_t *payload) {
   // if(!display_driver || !display_driver->draw_character) return;
    uint8_t row = payload[0];
    uint8_t col = payload[1];
    uint8_t attrs = payload[2]; // INAV and Betaflight use this to specify a higher page number. 
    uint8_t str_len;
    for(str_len = 1; str_len < 255; str_len++) {
        if(payload[2 + str_len] == '\0') {
            break;
        }
    }
    for(uint8_t idx = 0; idx < (str_len - 1); idx++) {
        uint16_t character = payload[3 + idx];
        if(attrs & 0x3) {
            // shift over by the page number if they were specified
            character |= ((attrs & 0x3) * 0x100);
        }
        //display_driver->draw_character(col, row, character);
        draw_character(row, col, character);
        col++;
    }
}

static void process_clear_screen(displayport_vtable_t *display_driver) {
    //if(!display_driver || !display_driver->clear_screen) return;
    //display_driver->clear_screen();
    clear_screen();
}

static void process_draw_complete(displayport_vtable_t *display_driver) {
    if(!display_driver || !display_driver->draw_complete) return;
    display_driver->draw_complete();
}

static void process_set_options(displayport_vtable_t *display_driver, uint8_t *payload) {
    if(!display_driver || !display_driver->set_options) return;
    uint8_t font = payload[0];
    msp_hd_options_e is_hd = payload[1];
    display_driver->set_options(font, is_hd);
}

static void process_open(displayport_vtable_t *display_driver) {

}

static void process_close(displayport_vtable_t *display_driver) {
    process_clear_screen(display_driver);
}

int displayport_process_message(displayport_vtable_t *display_driver, msp_msg_t *msg) {
    if (msg->direction != MSP_INBOUND) {
        return 1;
    }
    if (msg->cmd != MSP_CMD_DISPLAYPORT) {
        return 1;
    }
    msp_displayport_cmd_e sub_cmd = msg->payload[0];
    switch(sub_cmd) {
        case MSP_DISPLAYPORT_KEEPALIVE: // 0 -> Open/Keep-Alive DisplayPort
            process_open(display_driver);
            break;
        case MSP_DISPLAYPORT_CLOSE: // 1 -> Close DisplayPort
            process_close(display_driver);
            break;
        case MSP_DISPLAYPORT_CLEAR: // 2 -> Clear Screen
            process_clear_screen(display_driver);
            break;
        case MSP_DISPLAYPORT_DRAW_STRING: // 3 -> Draw String
            process_draw_string(display_driver, &msg->payload[1]);
            break;
        case MSP_DISPLAYPORT_DRAW_SCREEN: // 4 -> Draw Screen
            process_draw_complete(display_driver);
            break;
        case MSP_DISPLAYPORT_SET_OPTIONS: // 5 -> Set Options (HDZero/iNav)
            process_set_options(display_driver, &msg->payload[1]);
            break;
        default:
            break;
    }
    return 0;
}

