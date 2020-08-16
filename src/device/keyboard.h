/**
 * PS/2 keyboard input support.
 */


#ifndef KEYBOARD_H
#define KEYBOARD_H


#include <stdint.h>


/** A partial set of special keys on US QWERTY keyboard. */
enum keyboard_meta_key {
    KEY_ESC,        // Escape
    KEY_BACK,       // Backspace
    KEY_TAB,        // Tab
    KEY_ENTER,      // Enter
    KEY_CTRL,       // Both ctrls
    KEY_SHIFT,      // Both shifts
    KEY_ALT,        // Both alts
    KEY_CAPS,        // Capslock
    KEY_NULL,       // Dummy placeholder for empty key
};
typedef enum keyboard_meta_key keyboard_meta_key_t;

/** Holds info for a keyboard key. */
union keyboard_key_info {
    keyboard_meta_key_t meta;   /** Special meta key. */
    char code;                  /** ASCII byte code - lower case. */
};
typedef union keyboard_key_info keyboard_key_info_t;

/** Struct for a keyboard event. */
struct keyboard_key_event {
    bool press;     /** False if is a release event. */
    bool ascii;     /** True if is ASCII character, otherwise special. */
    keyboard_key_info_t info;
};
typedef struct keyboard_key_event keyboard_key_event_t;


void keyboard_init();


#endif
