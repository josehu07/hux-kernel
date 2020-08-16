/**
 * PS/2 keyboard input support.
 */


#include <stdint.h>
#include <stdbool.h>

#include "keyboard.h"

#include "../common/port.h"
#include "../common/printf.h"

#include "../display/vga.h"

#include "../interrupt/isr.h"


/**
 * Hardcoded scancode -> key event mapping.
 *
 * Check out https://wiki.osdev.org/Keyboard#Scan_Code_Set_1
 * for a complete list of mappings.
 *
 * We will only code a partial set of mappings - only those most
 * useful events. Notice that we are using the GNU-extended range syntax
 * of a switch-case statement.
 *
 * No multimedia part support.
 */
#define NO_KEY { .press = false, .ascii = false, .info = { .meta = KEY_NULL } }

static keyboard_key_event_t scancode_event_map[224] = {
    NO_KEY,                                                            // 0x00
    { .press = true , .ascii = false, .info = { .meta = KEY_ESC   } }, // 0x01
    { .press = true , .ascii = true , .info = { .code = '1'       } }, // 0x02
    { .press = true , .ascii = true , .info = { .code = '2'       } }, // 0x03
    { .press = true , .ascii = true , .info = { .code = '3'       } }, // 0x04
    { .press = true , .ascii = true , .info = { .code = '4'       } }, // 0x05
    { .press = true , .ascii = true , .info = { .code = '5'       } }, // 0x06
    { .press = true , .ascii = true , .info = { .code = '6'       } }, // 0x07
    { .press = true , .ascii = true , .info = { .code = '7'       } }, // 0x08
    { .press = true , .ascii = true , .info = { .code = '8'       } }, // 0x09
    { .press = true , .ascii = true , .info = { .code = '9'       } }, // 0x0A
    { .press = true , .ascii = true , .info = { .code = '0'       } }, // 0x0B
    { .press = true , .ascii = true , .info = { .code = '-'       } }, // 0x0C
    { .press = true , .ascii = true , .info = { .code = '='       } }, // 0x0D
    { .press = true , .ascii = false, .info = { .meta = KEY_BACK  } }, // 0x0E
    { .press = true , .ascii = false, .info = { .meta = KEY_TAB   } }, // 0x0F
    { .press = true , .ascii = true , .info = { .code = 'q'       } }, // 0x10
    { .press = true , .ascii = true , .info = { .code = 'w'       } }, // 0x11
    { .press = true , .ascii = true , .info = { .code = 'e'       } }, // 0x12
    { .press = true , .ascii = true , .info = { .code = 'r'       } }, // 0x13
    { .press = true , .ascii = true , .info = { .code = 't'       } }, // 0x14
    { .press = true , .ascii = true , .info = { .code = 'y'       } }, // 0x15
    { .press = true , .ascii = true , .info = { .code = 'u'       } }, // 0x16
    { .press = true , .ascii = true , .info = { .code = 'i'       } }, // 0x17
    { .press = true , .ascii = true , .info = { .code = 'o'       } }, // 0x18
    { .press = true , .ascii = true , .info = { .code = 'p'       } }, // 0x19
    { .press = true , .ascii = true , .info = { .code = '['       } }, // 0x1A
    { .press = true , .ascii = true , .info = { .code = ']'       } }, // 0x1B
    { .press = true , .ascii = false, .info = { .meta = KEY_ENTER } }, // 0x1C
    { .press = true , .ascii = false, .info = { .meta = KEY_CTRL  } }, // 0x1D
    { .press = true , .ascii = true , .info = { .code = 'a'       } }, // 0x1E
    { .press = true , .ascii = true , .info = { .code = 's'       } }, // 0x1F
    { .press = true , .ascii = true , .info = { .code = 'd'       } }, // 0x20
    { .press = true , .ascii = true , .info = { .code = 'f'       } }, // 0x21
    { .press = true , .ascii = true , .info = { .code = 'g'       } }, // 0x22
    { .press = true , .ascii = true , .info = { .code = 'h'       } }, // 0x23
    { .press = true , .ascii = true , .info = { .code = 'j'       } }, // 0x24
    { .press = true , .ascii = true , .info = { .code = 'k'       } }, // 0x25
    { .press = true , .ascii = true , .info = { .code = 'l'       } }, // 0x26
    { .press = true , .ascii = true , .info = { .code = ';'       } }, // 0x27
    { .press = true , .ascii = true , .info = { .code = '\''      } }, // 0x28
    { .press = true , .ascii = true , .info = { .code = '`'       } }, // 0x29
    { .press = true , .ascii = false, .info = { .meta = KEY_SHIFT } }, // 0x2A
    { .press = true , .ascii = true , .info = { .code = '\\'      } }, // 0x2B
    { .press = true , .ascii = true , .info = { .code = 'z'       } }, // 0x2C
    { .press = true , .ascii = true , .info = { .code = 'x'       } }, // 0x2D
    { .press = true , .ascii = true , .info = { .code = 'c'       } }, // 0x2E
    { .press = true , .ascii = true , .info = { .code = 'v'       } }, // 0x2F
    { .press = true , .ascii = true , .info = { .code = 'b'       } }, // 0x30
    { .press = true , .ascii = true , .info = { .code = 'n'       } }, // 0x31
    { .press = true , .ascii = true , .info = { .code = 'm'       } }, // 0x32
    { .press = true , .ascii = true , .info = { .code = ','       } }, // 0x33
    { .press = true , .ascii = true , .info = { .code = '.'       } }, // 0x34
    { .press = true , .ascii = true , .info = { .code = '/'       } }, // 0x35
    { .press = true , .ascii = false, .info = { .meta = KEY_SHIFT } }, // 0x36
    NO_KEY,                                                            // 0x37
    { .press = true , .ascii = false, .info = { .meta = KEY_ALT   } }, // 0x38
    { .press = true , .ascii = true , .info = { .code = ' '       } }, // 0x39
    { .press = true , .ascii = false, .info = { .meta = KEY_CAPS  } }, // 0x3A
    NO_KEY,                                                            // 0x3B
    NO_KEY,                                                            // 0x3C
    NO_KEY,                                                            // 0x3D
    NO_KEY,                                                            // 0x3E
    NO_KEY,                                                            // 0x3F
    NO_KEY,                                                            // 0x40
    NO_KEY,                                                            // 0x41
    NO_KEY,                                                            // 0x42
    NO_KEY,                                                            // 0x43
    NO_KEY,                                                            // 0x44
    NO_KEY,                                                            // 0x45
    NO_KEY,                                                            // 0x46
    NO_KEY,                                                            // 0x47
    NO_KEY,                                                            // 0x48
    NO_KEY,                                                            // 0x49
    NO_KEY,                                                            // 0x4A
    NO_KEY,                                                            // 0x4B
    NO_KEY,                                                            // 0x4C
    NO_KEY,                                                            // 0x4D
    NO_KEY,                                                            // 0x4E
    NO_KEY,                                                            // 0x4F
    NO_KEY,                                                            // 0x50
    NO_KEY,                                                            // 0x51
    NO_KEY,                                                            // 0x52
    NO_KEY,                                                            // 0x53
    NO_KEY,                                                            // 0x54
    NO_KEY,                                                            // 0x55
    NO_KEY,                                                            // 0x56
    NO_KEY,                                                            // 0x57
    NO_KEY,                                                            // 0x58
    NO_KEY,                                                            // 0x59
    NO_KEY,                                                            // 0x5A
    NO_KEY,                                                            // 0x5B
    NO_KEY,                                                            // 0x5C
    NO_KEY,                                                            // 0x5D
    NO_KEY,                                                            // 0x5E
    NO_KEY,                                                            // 0x5F
    NO_KEY,                                                            // 0x60
    NO_KEY,                                                            // 0x61
    NO_KEY,                                                            // 0x62
    NO_KEY,                                                            // 0x63
    NO_KEY,                                                            // 0x64
    NO_KEY,                                                            // 0x65
    NO_KEY,                                                            // 0x66
    NO_KEY,                                                            // 0x67
    NO_KEY,                                                            // 0x68
    NO_KEY,                                                            // 0x69
    NO_KEY,                                                            // 0x6A
    NO_KEY,                                                            // 0x6B
    NO_KEY,                                                            // 0x6C
    NO_KEY,                                                            // 0x6D
    NO_KEY,                                                            // 0x6E
    NO_KEY,                                                            // 0x6F
    NO_KEY,                                                            // 0x70
    NO_KEY,                                                            // 0x71
    NO_KEY,                                                            // 0x72
    NO_KEY,                                                            // 0x73
    NO_KEY,                                                            // 0x74
    NO_KEY,                                                            // 0x75
    NO_KEY,                                                            // 0x76
    NO_KEY,                                                            // 0x77
    NO_KEY,                                                            // 0x78
    NO_KEY,                                                            // 0x79
    NO_KEY,                                                            // 0x7A
    NO_KEY,                                                            // 0x7B
    NO_KEY,                                                            // 0x7C
    NO_KEY,                                                            // 0x7D
    NO_KEY,                                                            // 0x7E
    NO_KEY,                                                            // 0x7F
    NO_KEY,                                                            // 0x80
    { .press = false, .ascii = false, .info = { .meta = KEY_ESC   } }, // 0x81
    { .press = false, .ascii = true , .info = { .code = '1'       } }, // 0x82
    { .press = false, .ascii = true , .info = { .code = '2'       } }, // 0x83
    { .press = false, .ascii = true , .info = { .code = '3'       } }, // 0x84
    { .press = false, .ascii = true , .info = { .code = '4'       } }, // 0x85
    { .press = false, .ascii = true , .info = { .code = '5'       } }, // 0x86
    { .press = false, .ascii = true , .info = { .code = '6'       } }, // 0x87
    { .press = false, .ascii = true , .info = { .code = '7'       } }, // 0x88
    { .press = false, .ascii = true , .info = { .code = '8'       } }, // 0x89
    { .press = false, .ascii = true , .info = { .code = '9'       } }, // 0x8A
    { .press = false, .ascii = true , .info = { .code = '0'       } }, // 0x8B
    { .press = false, .ascii = true , .info = { .code = '-'       } }, // 0x8C
    { .press = false, .ascii = true , .info = { .code = '='       } }, // 0x8D
    { .press = false, .ascii = false, .info = { .meta = KEY_BACK  } }, // 0x8E
    { .press = false, .ascii = false, .info = { .meta = KEY_TAB   } }, // 0x8F
    { .press = false, .ascii = true , .info = { .code = 'q'       } }, // 0x90
    { .press = false, .ascii = true , .info = { .code = 'w'       } }, // 0x91
    { .press = false, .ascii = true , .info = { .code = 'e'       } }, // 0x92
    { .press = false, .ascii = true , .info = { .code = 'r'       } }, // 0x93
    { .press = false, .ascii = true , .info = { .code = 't'       } }, // 0x94
    { .press = false, .ascii = true , .info = { .code = 'y'       } }, // 0x95
    { .press = false, .ascii = true , .info = { .code = 'u'       } }, // 0x96
    { .press = false, .ascii = true , .info = { .code = 'i'       } }, // 0x97
    { .press = false, .ascii = true , .info = { .code = 'o'       } }, // 0x98
    { .press = false, .ascii = true , .info = { .code = 'p'       } }, // 0x99
    { .press = false, .ascii = true , .info = { .code = '['       } }, // 0x9A
    { .press = false, .ascii = true , .info = { .code = ']'       } }, // 0x9B
    { .press = false, .ascii = false, .info = { .meta = KEY_ENTER } }, // 0x9C
    { .press = false, .ascii = false, .info = { .meta = KEY_CTRL  } }, // 0x9D
    { .press = false, .ascii = true , .info = { .code = 'a'       } }, // 0x9E
    { .press = false, .ascii = true , .info = { .code = 's'       } }, // 0x9F
    { .press = false, .ascii = true , .info = { .code = 'd'       } }, // 0xA0
    { .press = false, .ascii = true , .info = { .code = 'f'       } }, // 0xA1
    { .press = false, .ascii = true , .info = { .code = 'g'       } }, // 0xA2
    { .press = false, .ascii = true , .info = { .code = 'h'       } }, // 0xA3
    { .press = false, .ascii = true , .info = { .code = 'j'       } }, // 0xA4
    { .press = false, .ascii = true , .info = { .code = 'k'       } }, // 0xA5
    { .press = false, .ascii = true , .info = { .code = 'l'       } }, // 0xA6
    { .press = false, .ascii = true , .info = { .code = ';'       } }, // 0xA7
    { .press = false, .ascii = true , .info = { .code = '\''      } }, // 0xA8
    { .press = false, .ascii = true , .info = { .code = '`'       } }, // 0xA9
    { .press = false, .ascii = false, .info = { .meta = KEY_SHIFT } }, // 0xAA
    { .press = false, .ascii = true , .info = { .code = '\\'      } }, // 0xAB
    { .press = false, .ascii = true , .info = { .code = 'z'       } }, // 0xAC
    { .press = false, .ascii = true , .info = { .code = 'x'       } }, // 0xAD
    { .press = false, .ascii = true , .info = { .code = 'c'       } }, // 0xAE
    { .press = false, .ascii = true , .info = { .code = 'v'       } }, // 0xAF
    { .press = false, .ascii = true , .info = { .code = 'b'       } }, // 0xB0
    { .press = false, .ascii = true , .info = { .code = 'n'       } }, // 0xB1
    { .press = false, .ascii = true , .info = { .code = 'm'       } }, // 0xB2
    { .press = false, .ascii = true , .info = { .code = ','       } }, // 0xB3
    { .press = false, .ascii = true , .info = { .code = '.'       } }, // 0xB4
    { .press = false, .ascii = true , .info = { .code = '/'       } }, // 0xB5
    { .press = false, .ascii = false, .info = { .meta = KEY_SHIFT } }, // 0xB6
    NO_KEY,                                                            // 0xB7
    { .press = false, .ascii = false, .info = { .meta = KEY_ALT   } }, // 0xB8
    { .press = false, .ascii = true , .info = { .code = ' '       } }, // 0xB9
    { .press = false, .ascii = false, .info = { .meta = KEY_CAPS  } }, // 0xBA
    NO_KEY,                                                            // 0xBB
    NO_KEY,                                                            // 0xBC
    NO_KEY,                                                            // 0xBD
    NO_KEY,                                                            // 0xBE
    NO_KEY,                                                            // 0xBF
    NO_KEY,                                                            // 0xC0
    NO_KEY,                                                            // 0xC1
    NO_KEY,                                                            // 0xC2
    NO_KEY,                                                            // 0xC3
    NO_KEY,                                                            // 0xC4
    NO_KEY,                                                            // 0xC5
    NO_KEY,                                                            // 0xC6
    NO_KEY,                                                            // 0xC7
    NO_KEY,                                                            // 0xC8
    NO_KEY,                                                            // 0xC9
    NO_KEY,                                                            // 0xCA
    NO_KEY,                                                            // 0xCB
    NO_KEY,                                                            // 0xCC
    NO_KEY,                                                            // 0xCD
    NO_KEY,                                                            // 0xCE
    NO_KEY,                                                            // 0xCF
    NO_KEY,                                                            // 0xD0
    NO_KEY,                                                            // 0xD1
    NO_KEY,                                                            // 0xD2
    NO_KEY,                                                            // 0xD3
    NO_KEY,                                                            // 0xD4
    NO_KEY,                                                            // 0xD5
    NO_KEY,                                                            // 0xD6
    NO_KEY,                                                            // 0xD7
    NO_KEY,                                                            // 0xD8
    NO_KEY,                                                            // 0xD9
    NO_KEY,                                                            // 0xDA
    NO_KEY,                                                            // 0xDB
    NO_KEY,                                                            // 0xDC
    NO_KEY,                                                            // 0xDD
    NO_KEY,                                                            // 0xDE
    NO_KEY,                                                            // 0xDF
};


/**
 * Timer interrupt handler registered for IRQ # 0.
 * Currently just prints a tick message.
 */
static void
keyboard_interrupt_handler(interrupt_state_t *state)
{
    (void) state;   /** Unused. */
    
    /** Read our the event's scancode. */
    uint8_t scancode = inb(0x60);

    /**
     * Translate the scancode into a key event, following the scancode
     * set 1 mappings.
     */
    if (scancode < 224) {
        keyboard_key_event_t event = scancode_event_map[scancode];

        if (event.press && event.ascii)
            cprintf(VGA_COLOR_LIGHT_BROWN, "%c", event.info.code);
    }
}


/**
 * Initialize the PIT timer. Registers timer interrupt ISR handler, sets
 * PIT to run in mode 3 with given frequency in Hz.
 */
void
keyboard_init()
{
    /** Register timer interrupt ISR handler. */
    isr_register(33, &keyboard_interrupt_handler);
}
