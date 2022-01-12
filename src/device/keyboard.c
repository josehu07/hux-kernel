/**
 * PS/2 keyboard input support.
 */


#include <stdint.h>
#include <stdbool.h>

#include "keyboard.h"

#include "../common/port.h"
#include "../common/printf.h"
#include "../common/debug.h"
#include "../common/string.h"
#include "../common/spinlock.h"

#include "../display/vga.h"
#include "../display/terminal.h"

#include "../interrupt/isr.h"

#include "../process/process.h"
#include "../process/scheduler.h"


/**
 * Hardcode scancode -> key event mapping.
 *
 * Check out https://wiki.osdev.org/Keyboard#Scan_Code_Set_1
 * for a complete list of mappings.
 *
 * We will only code a partial set of mappings - only those most
 * useful events.
 */
#define NO_KEY { .press = false, .ascii = false, .info = { .meta = KEY_NULL } }

static keyboard_key_event_t scancode_event_map[0xE0] = {
    NO_KEY,                                                                      // 0x00
    { .press = true , .ascii = false, .info = { .meta = KEY_ESC   } },           // 0x01
    { .press = true , .ascii = true , .info = { .codel = '1' , .codeu = '!' } }, // 0x02
    { .press = true , .ascii = true , .info = { .codel = '2' , .codeu = '@' } }, // 0x03
    { .press = true , .ascii = true , .info = { .codel = '3' , .codeu = '#' } }, // 0x04
    { .press = true , .ascii = true , .info = { .codel = '4' , .codeu = '$' } }, // 0x05
    { .press = true , .ascii = true , .info = { .codel = '5' , .codeu = '%' } }, // 0x06
    { .press = true , .ascii = true , .info = { .codel = '6' , .codeu = '^' } }, // 0x07
    { .press = true , .ascii = true , .info = { .codel = '7' , .codeu = '&' } }, // 0x08
    { .press = true , .ascii = true , .info = { .codel = '8' , .codeu = '*' } }, // 0x09
    { .press = true , .ascii = true , .info = { .codel = '9' , .codeu = '(' } }, // 0x0A
    { .press = true , .ascii = true , .info = { .codel = '0' , .codeu = ')' } }, // 0x0B
    { .press = true , .ascii = true , .info = { .codel = '-' , .codeu = '_' } }, // 0x0C
    { .press = true , .ascii = true , .info = { .codel = '=' , .codeu = '+' } }, // 0x0D
    { .press = true , .ascii = false, .info = { .meta = KEY_BACK  } },           // 0x0E
    { .press = true , .ascii = false, .info = { .meta = KEY_TAB   } },           // 0x0F
    { .press = true , .ascii = true , .info = { .codel = 'q' , .codeu = 'Q' } }, // 0x10
    { .press = true , .ascii = true , .info = { .codel = 'w' , .codeu = 'W' } }, // 0x11
    { .press = true , .ascii = true , .info = { .codel = 'e' , .codeu = 'E' } }, // 0x12
    { .press = true , .ascii = true , .info = { .codel = 'r' , .codeu = 'R' } }, // 0x13
    { .press = true , .ascii = true , .info = { .codel = 't' , .codeu = 'T' } }, // 0x14
    { .press = true , .ascii = true , .info = { .codel = 'y' , .codeu = 'Y' } }, // 0x15
    { .press = true , .ascii = true , .info = { .codel = 'u' , .codeu = 'U' } }, // 0x16
    { .press = true , .ascii = true , .info = { .codel = 'i' , .codeu = 'I' } }, // 0x17
    { .press = true , .ascii = true , .info = { .codel = 'o' , .codeu = 'O' } }, // 0x18
    { .press = true , .ascii = true , .info = { .codel = 'p' , .codeu = 'P' } }, // 0x19
    { .press = true , .ascii = true , .info = { .codel = '[' , .codeu = '{' } }, // 0x1A
    { .press = true , .ascii = true , .info = { .codel = ']' , .codeu = '}' } }, // 0x1B
    { .press = true , .ascii = false, .info = { .meta = KEY_ENTER } },           // 0x1C
    { .press = true , .ascii = false, .info = { .meta = KEY_CTRL  } },           // 0x1D
    { .press = true , .ascii = true , .info = { .codel = 'a' , .codeu = 'A' } }, // 0x1E
    { .press = true , .ascii = true , .info = { .codel = 's' , .codeu = 'S' } }, // 0x1F
    { .press = true , .ascii = true , .info = { .codel = 'd' , .codeu = 'D' } }, // 0x20
    { .press = true , .ascii = true , .info = { .codel = 'f' , .codeu = 'F' } }, // 0x21
    { .press = true , .ascii = true , .info = { .codel = 'g' , .codeu = 'G' } }, // 0x22
    { .press = true , .ascii = true , .info = { .codel = 'h' , .codeu = 'H' } }, // 0x23
    { .press = true , .ascii = true , .info = { .codel = 'j' , .codeu = 'J' } }, // 0x24
    { .press = true , .ascii = true , .info = { .codel = 'k' , .codeu = 'K' } }, // 0x25
    { .press = true , .ascii = true , .info = { .codel = 'l' , .codeu = 'L' } }, // 0x26
    { .press = true , .ascii = true , .info = { .codel = ';' , .codeu = ':' } }, // 0x27
    { .press = true , .ascii = true , .info = { .codel = '\'', .codeu = '"' } }, // 0x28
    { .press = true , .ascii = true , .info = { .codel = '`' , .codeu = '~' } }, // 0x29
    { .press = true , .ascii = false, .info = { .meta = KEY_SHIFT } },           // 0x2A
    { .press = true , .ascii = true , .info = { .codel = '\\', .codeu = '|' } }, // 0x2B
    { .press = true , .ascii = true , .info = { .codel = 'z' , .codeu = 'Z' } }, // 0x2C
    { .press = true , .ascii = true , .info = { .codel = 'x' , .codeu = 'X' } }, // 0x2D
    { .press = true , .ascii = true , .info = { .codel = 'c' , .codeu = 'C' } }, // 0x2E
    { .press = true , .ascii = true , .info = { .codel = 'v' , .codeu = 'V' } }, // 0x2F
    { .press = true , .ascii = true , .info = { .codel = 'b' , .codeu = 'B' } }, // 0x30
    { .press = true , .ascii = true , .info = { .codel = 'n' , .codeu = 'N' } }, // 0x31
    { .press = true , .ascii = true , .info = { .codel = 'm' , .codeu = 'M' } }, // 0x32
    { .press = true , .ascii = true , .info = { .codel = ',' , .codeu = '<' } }, // 0x33
    { .press = true , .ascii = true , .info = { .codel = '.' , .codeu = '>' } }, // 0x34
    { .press = true , .ascii = true , .info = { .codel = '/' , .codeu = '?' } }, // 0x35
    { .press = true , .ascii = false, .info = { .meta = KEY_SHIFT } },           // 0x36
    NO_KEY,                                                                      // 0x37
    { .press = true , .ascii = false, .info = { .meta = KEY_ALT   } },           // 0x38
    { .press = true , .ascii = true , .info = { .codel = ' ' , .codeu = ' ' } }, // 0x39
    { .press = true , .ascii = false, .info = { .meta = KEY_CAPS  } },           // 0x3A
    NO_KEY,                                                                      // 0x3B
    NO_KEY,                                                                      // 0x3C
    NO_KEY,                                                                      // 0x3D
    NO_KEY,                                                                      // 0x3E
    NO_KEY,                                                                      // 0x3F
    NO_KEY,                                                                      // 0x40
    NO_KEY,                                                                      // 0x41
    NO_KEY,                                                                      // 0x42
    NO_KEY,                                                                      // 0x43
    NO_KEY,                                                                      // 0x44
    NO_KEY,                                                                      // 0x45
    NO_KEY,                                                                      // 0x46
    NO_KEY,                                                                      // 0x47
    NO_KEY,                                                                      // 0x48
    NO_KEY,                                                                      // 0x49
    NO_KEY,                                                                      // 0x4A
    NO_KEY,                                                                      // 0x4B
    NO_KEY,                                                                      // 0x4C
    NO_KEY,                                                                      // 0x4D
    NO_KEY,                                                                      // 0x4E
    NO_KEY,                                                                      // 0x4F
    NO_KEY,                                                                      // 0x50
    NO_KEY,                                                                      // 0x51
    NO_KEY,                                                                      // 0x52
    NO_KEY,                                                                      // 0x53
    NO_KEY,                                                                      // 0x54
    NO_KEY,                                                                      // 0x55
    NO_KEY,                                                                      // 0x56
    NO_KEY,                                                                      // 0x57
    NO_KEY,                                                                      // 0x58
    NO_KEY,                                                                      // 0x59
    NO_KEY,                                                                      // 0x5A
    NO_KEY,                                                                      // 0x5B
    NO_KEY,                                                                      // 0x5C
    NO_KEY,                                                                      // 0x5D
    NO_KEY,                                                                      // 0x5E
    NO_KEY,                                                                      // 0x5F
    NO_KEY,                                                                      // 0x60
    NO_KEY,                                                                      // 0x61
    NO_KEY,                                                                      // 0x62
    NO_KEY,                                                                      // 0x63
    NO_KEY,                                                                      // 0x64
    NO_KEY,                                                                      // 0x65
    NO_KEY,                                                                      // 0x66
    NO_KEY,                                                                      // 0x67
    NO_KEY,                                                                      // 0x68
    NO_KEY,                                                                      // 0x69
    NO_KEY,                                                                      // 0x6A
    NO_KEY,                                                                      // 0x6B
    NO_KEY,                                                                      // 0x6C
    NO_KEY,                                                                      // 0x6D
    NO_KEY,                                                                      // 0x6E
    NO_KEY,                                                                      // 0x6F
    NO_KEY,                                                                      // 0x70
    NO_KEY,                                                                      // 0x71
    NO_KEY,                                                                      // 0x72
    NO_KEY,                                                                      // 0x73
    NO_KEY,                                                                      // 0x74
    NO_KEY,                                                                      // 0x75
    NO_KEY,                                                                      // 0x76
    NO_KEY,                                                                      // 0x77
    NO_KEY,                                                                      // 0x78
    NO_KEY,                                                                      // 0x79
    NO_KEY,                                                                      // 0x7A
    NO_KEY,                                                                      // 0x7B
    NO_KEY,                                                                      // 0x7C
    NO_KEY,                                                                      // 0x7D
    NO_KEY,                                                                      // 0x7E
    NO_KEY,                                                                      // 0x7F
    NO_KEY,                                                                      // 0x80
    { .press = false, .ascii = false, .info = { .meta = KEY_ESC   } },           // 0x81
    { .press = false, .ascii = true , .info = { .codel = '1' , .codeu = '!' } }, // 0x82
    { .press = false, .ascii = true , .info = { .codel = '2' , .codeu = '@' } }, // 0x83
    { .press = false, .ascii = true , .info = { .codel = '3' , .codeu = '#' } }, // 0x84
    { .press = false, .ascii = true , .info = { .codel = '4' , .codeu = '$' } }, // 0x85
    { .press = false, .ascii = true , .info = { .codel = '5' , .codeu = '%' } }, // 0x86
    { .press = false, .ascii = true , .info = { .codel = '6' , .codeu = '^' } }, // 0x87
    { .press = false, .ascii = true , .info = { .codel = '7' , .codeu = '&' } }, // 0x88
    { .press = false, .ascii = true , .info = { .codel = '8' , .codeu = '*' } }, // 0x89
    { .press = false, .ascii = true , .info = { .codel = '9' , .codeu = '(' } }, // 0x8A
    { .press = false, .ascii = true , .info = { .codel = '0' , .codeu = ')' } }, // 0x8B
    { .press = false, .ascii = true , .info = { .codel = '-' , .codeu = '_' } }, // 0x8C
    { .press = false, .ascii = true , .info = { .codel = '=' , .codeu = '+' } }, // 0x8D
    { .press = false, .ascii = false, .info = { .meta = KEY_BACK  } },           // 0x8E
    { .press = false, .ascii = false, .info = { .meta = KEY_TAB   } },           // 0x8F
    { .press = false, .ascii = true , .info = { .codel = 'q' , .codeu = 'Q' } }, // 0x90
    { .press = false, .ascii = true , .info = { .codel = 'w' , .codeu = 'W' } }, // 0x91
    { .press = false, .ascii = true , .info = { .codel = 'e' , .codeu = 'E' } }, // 0x92
    { .press = false, .ascii = true , .info = { .codel = 'r' , .codeu = 'R' } }, // 0x93
    { .press = false, .ascii = true , .info = { .codel = 't' , .codeu = 'T' } }, // 0x94
    { .press = false, .ascii = true , .info = { .codel = 'y' , .codeu = 'Y' } }, // 0x95
    { .press = false, .ascii = true , .info = { .codel = 'u' , .codeu = 'U' } }, // 0x96
    { .press = false, .ascii = true , .info = { .codel = 'i' , .codeu = 'I' } }, // 0x97
    { .press = false, .ascii = true , .info = { .codel = 'o' , .codeu = 'O' } }, // 0x98
    { .press = false, .ascii = true , .info = { .codel = 'p' , .codeu = 'P' } }, // 0x99
    { .press = false, .ascii = true , .info = { .codel = '[' , .codeu = '{' } }, // 0x9A
    { .press = false, .ascii = true , .info = { .codel = ']' , .codeu = '}' } }, // 0x9B
    { .press = false, .ascii = false, .info = { .meta = KEY_ENTER } },           // 0x9C
    { .press = false, .ascii = false, .info = { .meta = KEY_CTRL  } },           // 0x9D
    { .press = false, .ascii = true , .info = { .codel = 'a' , .codeu = 'A' } }, // 0x9E
    { .press = false, .ascii = true , .info = { .codel = 's' , .codeu = 'S' } }, // 0x9F
    { .press = false, .ascii = true , .info = { .codel = 'd' , .codeu = 'D' } }, // 0xA0
    { .press = false, .ascii = true , .info = { .codel = 'f' , .codeu = 'F' } }, // 0xA1
    { .press = false, .ascii = true , .info = { .codel = 'g' , .codeu = 'G' } }, // 0xA2
    { .press = false, .ascii = true , .info = { .codel = 'h' , .codeu = 'H' } }, // 0xA3
    { .press = false, .ascii = true , .info = { .codel = 'j' , .codeu = 'J' } }, // 0xA4
    { .press = false, .ascii = true , .info = { .codel = 'k' , .codeu = 'K' } }, // 0xA5
    { .press = false, .ascii = true , .info = { .codel = 'l' , .codeu = 'L' } }, // 0xA6
    { .press = false, .ascii = true , .info = { .codel = ';' , .codeu = ':' } }, // 0xA7
    { .press = false, .ascii = true , .info = { .codel = '\'', .codeu = '"' } }, // 0xA8
    { .press = false, .ascii = true , .info = { .codel = '`' , .codeu = '~' } }, // 0xA9
    { .press = false, .ascii = false, .info = { .meta = KEY_SHIFT } },           // 0xAA
    { .press = false, .ascii = true , .info = { .codel = '\\', .codeu = '|' } }, // 0xAB
    { .press = false, .ascii = true , .info = { .codel = 'z' , .codeu = 'Z' } }, // 0xAC
    { .press = false, .ascii = true , .info = { .codel = 'x' , .codeu = 'X' } }, // 0xAD
    { .press = false, .ascii = true , .info = { .codel = 'c' , .codeu = 'C' } }, // 0xAE
    { .press = false, .ascii = true , .info = { .codel = 'v' , .codeu = 'V' } }, // 0xAF
    { .press = false, .ascii = true , .info = { .codel = 'b' , .codeu = 'B' } }, // 0xB0
    { .press = false, .ascii = true , .info = { .codel = 'n' , .codeu = 'N' } }, // 0xB1
    { .press = false, .ascii = true , .info = { .codel = 'm' , .codeu = 'M' } }, // 0xB2
    { .press = false, .ascii = true , .info = { .codel = ',' , .codeu = '<' } }, // 0xB3
    { .press = false, .ascii = true , .info = { .codel = '.' , .codeu = '>' } }, // 0xB4
    { .press = false, .ascii = true , .info = { .codel = '/' , .codeu = '?' } }, // 0xB5
    { .press = false, .ascii = false, .info = { .meta = KEY_SHIFT } },           // 0xB6
    NO_KEY,                                                                      // 0xB7
    { .press = false, .ascii = false, .info = { .meta = KEY_ALT   } },           // 0xB8
    { .press = false, .ascii = true , .info = { .codel = ' ' , .codeu = ' ' } }, // 0xB9
    { .press = false, .ascii = false, .info = { .meta = KEY_CAPS  } },           // 0xBA
    NO_KEY,                                                                      // 0xBB
    NO_KEY,                                                                      // 0xBC
    NO_KEY,                                                                      // 0xBD
    NO_KEY,                                                                      // 0xBE
    NO_KEY,                                                                      // 0xBF
    NO_KEY,                                                                      // 0xC0
    NO_KEY,                                                                      // 0xC1
    NO_KEY,                                                                      // 0xC2
    NO_KEY,                                                                      // 0xC3
    NO_KEY,                                                                      // 0xC4
    NO_KEY,                                                                      // 0xC5
    NO_KEY,                                                                      // 0xC6
    NO_KEY,                                                                      // 0xC7
    NO_KEY,                                                                      // 0xC8
    NO_KEY,                                                                      // 0xC9
    NO_KEY,                                                                      // 0xCA
    NO_KEY,                                                                      // 0xCB
    NO_KEY,                                                                      // 0xCC
    NO_KEY,                                                                      // 0xCD
    NO_KEY,                                                                      // 0xCE
    NO_KEY,                                                                      // 0xCF
    NO_KEY,                                                                      // 0xD0
    NO_KEY,                                                                      // 0xD1
    NO_KEY,                                                                      // 0xD2
    NO_KEY,                                                                      // 0xD3
    NO_KEY,                                                                      // 0xD4
    NO_KEY,                                                                      // 0xD5
    NO_KEY,                                                                      // 0xD6
    NO_KEY,                                                                      // 0xD7
    NO_KEY,                                                                      // 0xD8
    NO_KEY,                                                                      // 0xD9
    NO_KEY,                                                                      // 0xDA
    NO_KEY,                                                                      // 0xDB
    NO_KEY,                                                                      // 0xDC
    NO_KEY,                                                                      // 0xDD
    NO_KEY,                                                                      // 0xDE
    NO_KEY,                                                                      // 0xDF
};

static keyboard_key_event_t extendcode_event_map[0xE0] = {
    NO_KEY,                                                            // 0x00
    NO_KEY,                                                            // 0x01
    NO_KEY,                                                            // 0x02
    NO_KEY,                                                            // 0x03
    NO_KEY,                                                            // 0x04
    NO_KEY,                                                            // 0x05
    NO_KEY,                                                            // 0x06
    NO_KEY,                                                            // 0x07
    NO_KEY,                                                            // 0x08
    NO_KEY,                                                            // 0x09
    NO_KEY,                                                            // 0x0A
    NO_KEY,                                                            // 0x0B
    NO_KEY,                                                            // 0x0C
    NO_KEY,                                                            // 0x0D
    NO_KEY,                                                            // 0x0E
    NO_KEY,                                                            // 0x0F
    NO_KEY,                                                            // 0x10
    NO_KEY,                                                            // 0x11
    NO_KEY,                                                            // 0x12
    NO_KEY,                                                            // 0x13
    NO_KEY,                                                            // 0x14
    NO_KEY,                                                            // 0x15
    NO_KEY,                                                            // 0x16
    NO_KEY,                                                            // 0x17
    NO_KEY,                                                            // 0x18
    NO_KEY,                                                            // 0x19
    NO_KEY,                                                            // 0x1A
    NO_KEY,                                                            // 0x1B
    NO_KEY,                                                            // 0x1C
    { .press = true , .ascii = false, .info = { .meta = KEY_CTRL  } }, // 0x1D
    NO_KEY,                                                            // 0x1E
    NO_KEY,                                                            // 0x1F
    NO_KEY,                                                            // 0x20
    NO_KEY,                                                            // 0x21
    NO_KEY,                                                            // 0x22
    NO_KEY,                                                            // 0x23
    NO_KEY,                                                            // 0x24
    NO_KEY,                                                            // 0x25
    NO_KEY,                                                            // 0x26
    NO_KEY,                                                            // 0x27
    NO_KEY,                                                            // 0x28
    NO_KEY,                                                            // 0x29
    NO_KEY,                                                            // 0x2A
    NO_KEY,                                                            // 0x2B
    NO_KEY,                                                            // 0x2C
    NO_KEY,                                                            // 0x2D
    NO_KEY,                                                            // 0x2E
    NO_KEY,                                                            // 0x2F
    NO_KEY,                                                            // 0x30
    NO_KEY,                                                            // 0x31
    NO_KEY,                                                            // 0x32
    NO_KEY,                                                            // 0x33
    NO_KEY,                                                            // 0x34
    NO_KEY,                                                            // 0x35
    NO_KEY,                                                            // 0x36
    NO_KEY,                                                            // 0x37
    { .press = true , .ascii = false, .info = { .meta = KEY_ALT   } }, // 0x38
    NO_KEY,                                                            // 0x39
    NO_KEY,                                                            // 0x3A
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
    { .press = true , .ascii = false, .info = { .meta = KEY_HOME  } }, // 0x47
    { .press = true , .ascii = false, .info = { .meta = KEY_UP    } }, // 0x48
    { .press = true , .ascii = false, .info = { .meta = KEY_PGUP  } }, // 0x49
    NO_KEY,                                                            // 0x4A
    { .press = true , .ascii = false, .info = { .meta = KEY_LEFT  } }, // 0x4B
    NO_KEY,                                                            // 0x4C
    { .press = true , .ascii = false, .info = { .meta = KEY_RIGHT } }, // 0x4D
    NO_KEY,                                                            // 0x4E
    { .press = true , .ascii = false, .info = { .meta = KEY_END   } }, // 0x4F
    { .press = true , .ascii = false, .info = { .meta = KEY_DOWN  } }, // 0x50
    { .press = true , .ascii = false, .info = { .meta = KEY_PGDN  } }, // 0x51
    { .press = true , .ascii = false, .info = { .meta = KEY_INS   } }, // 0x52
    { .press = true , .ascii = false, .info = { .meta = KEY_DEL   } }, // 0x53
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
    NO_KEY,                                                            // 0x81
    NO_KEY,                                                            // 0x82
    NO_KEY,                                                            // 0x83
    NO_KEY,                                                            // 0x84
    NO_KEY,                                                            // 0x85
    NO_KEY,                                                            // 0x86
    NO_KEY,                                                            // 0x87
    NO_KEY,                                                            // 0x88
    NO_KEY,                                                            // 0x89
    NO_KEY,                                                            // 0x8A
    NO_KEY,                                                            // 0x8B
    NO_KEY,                                                            // 0x8C
    NO_KEY,                                                            // 0x8D
    NO_KEY,                                                            // 0x8E
    NO_KEY,                                                            // 0x8F
    NO_KEY,                                                            // 0x90
    NO_KEY,                                                            // 0x91
    NO_KEY,                                                            // 0x92
    NO_KEY,                                                            // 0x93
    NO_KEY,                                                            // 0x94
    NO_KEY,                                                            // 0x95
    NO_KEY,                                                            // 0x96
    NO_KEY,                                                            // 0x97
    NO_KEY,                                                            // 0x98
    NO_KEY,                                                            // 0x99
    NO_KEY,                                                            // 0x9A
    NO_KEY,                                                            // 0x9B
    NO_KEY,                                                            // 0x9C
    { .press = false, .ascii = false, .info = { .meta = KEY_CTRL  } }, // 0x9D
    NO_KEY,                                                            // 0xA0
    NO_KEY,                                                            // 0xA1
    NO_KEY,                                                            // 0xA2
    NO_KEY,                                                            // 0xA3
    NO_KEY,                                                            // 0xA4
    NO_KEY,                                                            // 0xA5
    NO_KEY,                                                            // 0xA6
    NO_KEY,                                                            // 0xA7
    NO_KEY,                                                            // 0xA8
    NO_KEY,                                                            // 0xA9
    NO_KEY,                                                            // 0xAA
    NO_KEY,                                                            // 0xAB
    NO_KEY,                                                            // 0xAC
    NO_KEY,                                                            // 0xAD
    NO_KEY,                                                            // 0xAE
    NO_KEY,                                                            // 0xAF
    NO_KEY,                                                            // 0xB0
    NO_KEY,                                                            // 0xB1
    NO_KEY,                                                            // 0xB2
    NO_KEY,                                                            // 0xB3
    NO_KEY,                                                            // 0xB4
    NO_KEY,                                                            // 0xB5
    NO_KEY,                                                            // 0xB6
    NO_KEY,                                                            // 0xB7
    { .press = false, .ascii = false, .info = { .meta = KEY_ALT   } }, // 0xB8
    NO_KEY,                                                            // 0xB9
    NO_KEY,                                                            // 0xBA
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
    { .press = false, .ascii = false, .info = { .meta = KEY_HOME  } }, // 0xC7
    { .press = false, .ascii = false, .info = { .meta = KEY_UP    } }, // 0xC8
    { .press = false, .ascii = false, .info = { .meta = KEY_PGUP  } }, // 0xC9
    NO_KEY,                                                            // 0xCA
    { .press = false, .ascii = false, .info = { .meta = KEY_LEFT  } }, // 0xCB
    NO_KEY,                                                            // 0xCC
    { .press = false, .ascii = false, .info = { .meta = KEY_RIGHT } }, // 0xCD
    NO_KEY,                                                            // 0xCE
    { .press = false, .ascii = false, .info = { .meta = KEY_END   } }, // 0xCF
    { .press = false, .ascii = false, .info = { .meta = KEY_DOWN  } }, // 0xD0
    { .press = false, .ascii = false, .info = { .meta = KEY_PGDN  } }, // 0xD1
    { .press = false, .ascii = false, .info = { .meta = KEY_INS   } }, // 0xD2
    { .press = false, .ascii = false, .info = { .meta = KEY_DEL   } }, // 0xD3
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


/** A circular buffer for recording the input string from keyboard. */
#define INPUT_BUF_SIZE 256
static char input_buf[INPUT_BUF_SIZE];

/**
 * These two numbers grow indefinitely. `loc % INPUT_BUF_SIZE` is the
 * actual index in the circular buffer.
 */
static size_t input_put_loc = 0;   // Place to record the next char.
static size_t input_get_loc = 0;   // Start of the first unfetched char.

/** Upper case triggers, both on means lower case. */
static bool shift_held = false;
static bool capslock_on = false;


/** If not NULL, that process is listening on keyboard events. */
static process_t *listener_proc = NULL;

static spinlock_t keyboard_lock;


/**
 * Keyboard interrupt handler registered for IRQ #1.
 * Serves keyboard input requests. Interrupts should have been disabled
 * automatically since this is an interrupt gate.
 *
 * Currently only supports lower cased ASCII characters, upper case by
 * holding SHIFT or activating CAPSLOCK, and newline. Assumes that at
 * most one process could be listening on keyboard input at the same time.
 */
static void
keyboard_interrupt_handler(interrupt_state_t *state)
{
    (void) state;   /** Unused. */
    
    keyboard_key_event_t event = NO_KEY;

    /**
     * Read our the event's scancode. Translate the scancode into a key
     * event, following the scancode set 1 mappings.
     */
    uint8_t scancode = inb(0x60);
    if (scancode < 0xE0)
        event = scancode_event_map[scancode];
    else if (scancode == 0xE0) {    /** Is a key in extended set. */
        uint8_t extendcode = inb(0x60);
        if (extendcode < 0xE0)
            event = extendcode_event_map[extendcode];
    }

    // if (event.press && event.ascii)
    //     printf("%c", event.info.codel);

    spinlock_acquire(&keyboard_lock);

    /**
     * React only if no overwriting could happen and if a process is
     * listening on keyboard input. Record the char to the circular buffer,
     * unblock it when buffer is full or when an ENTER press happens.
     * Interactively displays the character.
     */
    if (input_put_loc - input_get_loc < INPUT_BUF_SIZE
        && listener_proc != NULL && listener_proc->state == BLOCKED
        && listener_proc->block_on == ON_KBDIN) {
        bool is_enter = !event.ascii && event.info.meta == KEY_ENTER;
        bool is_back = !event.ascii && event.info.meta == KEY_BACK;
        bool is_shift = !event.ascii && event.info.meta == KEY_SHIFT;
        bool is_caps = !event.ascii && event.info.meta == KEY_CAPS;

        if (!shift_held && event.press && is_shift)
            shift_held = true;
        else if (shift_held && !event.press && is_shift)
            shift_held = false;
        capslock_on = (event.press && is_caps) ? !capslock_on : capslock_on;
        bool upper_case = (shift_held != capslock_on);

        if (event.press && (event.ascii || is_enter)) {
            char c = !event.ascii ? '\n' :
                     upper_case ? event.info.codeu : event.info.codel;
            input_buf[(input_put_loc++) % INPUT_BUF_SIZE] = c;
            printf("%c", c);
        } else if (event.press && is_back) {
            if (input_put_loc > input_get_loc) {
                input_put_loc--;
                spinlock_acquire(&terminal_lock);
                terminal_erase();
                spinlock_release(&terminal_lock);
            }
        }

        if ((event.press && is_enter)
            || input_put_loc >= input_get_loc + INPUT_BUF_SIZE) {
            spinlock_acquire(&ptable_lock);
            process_unblock(listener_proc);
            spinlock_release(&ptable_lock);
        }
    }

    spinlock_release(&keyboard_lock);
}


/** Initialize the PS/2 keyboard device. */
void
keyboard_init()
{
    memset(input_buf, 0, sizeof(char) * INPUT_BUF_SIZE);

    input_put_loc = 0;
    input_get_loc = 0;

    shift_held = false;
    capslock_on = false;

    listener_proc = NULL;

    spinlock_init(&keyboard_lock, "keyboard_lock");

    /** Register keyboard interrupt ISR handler. */
    isr_register(INT_NO_KEYBOARD, &keyboard_interrupt_handler);
}


/**
 * Listen on keyboard characters, interpret as an input string, and write the
 * string into the given buffer. Returns the length of the string actually
 * fetched, or -1 on errors.
 * 
 * The listening terminates on any of the following cases:
 *   - A total of `len - 1` bytes have been fetched;
 *   - Got a newline symbol.
 */
int32_t
keyboard_getstr(char *buf, size_t len)
{
    assert(buf != NULL);
    assert(len > 0);

    spinlock_acquire(&keyboard_lock);

    if (listener_proc != NULL) {
        warn("keyboard_getstr: there is already a keyboard listener");
        spinlock_release(&keyboard_lock);
        return -1;
    }

    process_t *proc = running_proc();
    listener_proc = proc;
    
    input_get_loc = input_put_loc;
    size_t left = len;

    while (left > 1) {
        /** Wait until there are unhandled chars. */
        while (input_get_loc == input_put_loc) {
            if (proc->killed) {
                spinlock_release(&keyboard_lock);
                return -1;
            }

            spinlock_acquire(&ptable_lock);
            spinlock_release(&keyboard_lock);

            process_block(ON_KBDIN);

            spinlock_release(&ptable_lock);
            spinlock_acquire(&keyboard_lock);
        }

        /** Fetch the next unhandled char. */
        char c = input_buf[(input_get_loc++) % INPUT_BUF_SIZE];
        buf[len - left] = c;
        left--;

        if (c == '\n')  /** Newline triggers early break. */
            break;
    }

    /** Fill a null-terminator to finish the string. */
    size_t fetched = len - left;
    buf[fetched] = '\0';

    /** Clear the listener. */
    listener_proc = NULL;

    spinlock_release(&keyboard_lock);
    return fetched;
}
