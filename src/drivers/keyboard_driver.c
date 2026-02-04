#include "kernel/print.h"
#include "kernel/tty.h"
#include "drivers/keyboard_driver.h"
#include "io/port.h"
#include "utils.h"
#include "errno.h"

// Each index is the PS/2 Set 1 scan code
// 0xFF = unmapped / unused.
static uint8_t scancode_to_key_index[256] = {
    [0 ... 255] = 0xFF, // mark all unmapped first
    
    // 0x00 – 0x0F
    [0x01] = KEY_ESC,
    [0x02] = KEY_1,
    [0x03] = KEY_2,
    [0x04] = KEY_3,
    [0x05] = KEY_4,
    [0x06] = KEY_5,
    [0x07] = KEY_6,
    [0x08] = KEY_7,
    [0x09] = KEY_8,
    [0x0A] = KEY_9,
    [0x0B] = KEY_0,
    [0x0C] = KEY_MINUS,
    [0x0D] = KEY_EQUAL,
    [0x0E] = KEY_BACKSPACE,
    [0x0F] = KEY_TAB,

    // 0x10 – 0x1F
    [0x10] = KEY_Q,
    [0x11] = KEY_W,
    [0x12] = KEY_E,
    [0x13] = KEY_R,
    [0x14] = KEY_T,
    [0x15] = KEY_Y,
    [0x16] = KEY_U,
    [0x17] = KEY_I,
    [0x18] = KEY_O,
    [0x19] = KEY_P,
    [0x1A] = KEY_LBRACKET,
    [0x1B] = KEY_RBRACKET,
    [0x1C] = KEY_ENTER,
    [0x1D] = KEY_LCTRL, // Right Ctrl has 0xE0 prefix
    [0x1E] = KEY_A,
    [0x1F] = KEY_S,

    // 0x20 – 0x2F
    [0x20] = KEY_D,
    [0x21] = KEY_F,
    [0x22] = KEY_G,
    [0x23] = KEY_H,
    [0x24] = KEY_J,
    [0x25] = KEY_K,
    [0x26] = KEY_L,
    [0x27] = KEY_SEMICOLON,
    [0x28] = KEY_APOSTROPHE,
    [0x29] = KEY_GRAVE,
    [0x2A] = KEY_LSHIFT,
    [0x2B] = KEY_BACKSLASH,
    [0x2C] = KEY_Z,
    [0x2D] = KEY_X,
    [0x2E] = KEY_C,
    [0x2F] = KEY_V,

    // 0x30 – 0x3F
    [0x30] = KEY_B,
    [0x31] = KEY_N,
    [0x32] = KEY_M,
    [0x33] = KEY_COMMA,
    [0x34] = KEY_PERIOD,
    [0x35] = KEY_SLASH,
    [0x36] = KEY_RSHIFT,
    [0x37] = KEY_KP_MUL,    // on keypad (*)
    [0x38] = KEY_LALT,
    [0x39] = KEY_SPACE,
    [0x3A] = KEY_CAPSLOCK,
    [0x3B] = KEY_F1,
    [0x3C] = KEY_F2,
    [0x3D] = KEY_F3,
    [0x3E] = KEY_F4,
    [0x3F] = KEY_F5,

    // 0x40 – 0x4F
    [0x40] = KEY_F6,
    [0x41] = KEY_F7,
    [0x42] = KEY_F8,
    [0x43] = KEY_F9,
    [0x44] = KEY_F10,
    [0x45] = KEY_NUM_LOCK,
    [0x46] = KEY_SCROLL_LOCK,
    [0x47] = KEY_KP_7,
    [0x48] = KEY_KP_8,
    [0x49] = KEY_KP_9,
    [0x4A] = KEY_KP_MINUS,
    [0x4B] = KEY_KP_4,
    [0x4C] = KEY_KP_5,
    [0x4D] = KEY_KP_6,
    [0x4E] = KEY_KP_PLUS,
    [0x4F] = KEY_KP_1,

    // 0x50 – 0x5F
    [0x50] = KEY_KP_2,
    [0x51] = KEY_KP_3,
    [0x52] = KEY_KP_0,
    [0x53] = KEY_KP_DOT,
    [0x57] = KEY_F11,
    [0x58] = KEY_F12,
};

static unsigned char ex_code_to_key[128] = {
	0,              0,            0,          0,              0,            0,               0, 0,         /* 0x00-0x07 */
	0,              0,            0,          0,              0,            0,               0, 0,         /* 0x08-0x0F */
	0,              0,            0,          0,              0,            0,               0, 0,         /* 0x10-0x17 */
	0,              0,            0,          0,              KEY_KP_ENTER, KEY_RCTRL,       0, 0,         /* 0x18-0x1F */
	0,              0,            0,          0,              0,            0,               0, 0,         /* 0x20-0x27 */
	0,              0,            0,          0,              0,            0,               0, 0,         /* 0x28-0x2F */
	0,              0,            0,          0,              0,            KEY_SLASH,       0, 0,         /* 0x30-0x37 */
	KEY_RALT,       0,            0,          0,              0,            0,               0, 0,	       /* 0x38-0x3F */
	0,              0,            0,          0,              0,            0,               0, KEY_HOME,  /* 0x40-0x47 */
	KEY_ARROW_UP,   KEY_PAGEUP,   0,          KEY_ARROW_LEFT, 0,            KEY_ARROW_RIGHT, 0, KEY_END,   /* 0x48-0x4F */
	KEY_ARROW_DOWN, KEY_PAGEDOWN, KEY_INSERT, KEY_DELETE,     0,            0,               0, 0,	       /* 0x50-0x57 */
	0,              0,            0,          0,              0,            0,               0, 0,		   /* 0x58-0x5f */
	0,              0,            0,          0,              0,            0,               0, 0,		   /* 0x60-0x67 */
	0,              0,            0,          0,              0,            0,               0, 0,		   /* 0x68-0x6F */
	0,              0,            0,          0,              0,            0,               0, 0,		   /* 0x70-0x77 */
	0,              0,            0,          0,              0,            0,               0, 0		   /* 0x78-0x7F */
};

static const char key_ascii_map_shift_off[] = {
    0,      // KEY_NO_KEY = 0

    // Digits (top row)
    '0',    // 1  KEY_0
    '1',    // 2
    '2',    // 3
    '3',    // 4
    '4',    // 5
    '5',    // 6
    '6',    // 7
    '7',    // 8
    '8',    // 9
    '9',    // 0

    // Keypad digits
    '0',    // 11
    '1',    // 12
    '2',    // 13
    '3',    // 14
    '4',    // 15
    '5',    // 16
    '6',    // 17
    '7',    // 18
    '8',    // 19
    '9',    // 20

    // Letters (always lowercase here)
    'a',    // 21
    'b',    // 22
    'c',    // 23
    'd',    // 24
    'e',    // 25
    'f',    // 26
    'g',    // 27
    'h',    // 28
    'i',    // 29
    'j',    // 30
    'k',    // 31
    'l',    // 32
    'm',    // 33
    'n',    // 34
    'o',    // 35
    'p',    // 36
    'q',    // 37
    'r',    // 38
    's',    // 39
    't',    // 40
    'u',    // 41
    'v',    // 42
    'w',    // 43
    'x',    // 44
    'y',    // 45
    'z',    // 46

    // F1–F12 → 0 (non-printable)
    0,0,0,0,0,0,0,0,0,0,0,0,   // 47–58

    // Control keys
    0,      // 59  ESC
    '\t',   // 60  TAB
    0,0,0,0,0,0,0,           // 61–67
    ' ',    // 68  SPACE
    '\n',   // 69  ENTER
    0,      // 70  BACKSPACE is handled separately

    // Navigation keys: all 0
    0,0,0,0,0,0,0,0,0,         // 71–80

    // Keypad ops
    '+',    // 81
    '-',    // 82
    '*',    // 83
    '/',    // 84
    '\n',   // 85  KP_ENTER
    '.',    // 86  KP_DOT

    // Symbols
    '-',    // 87 KEY_MINUS
    '=',    // 88
    '[',    // 89
    ']',    // 90
    '\\',   // 91
    ';',    // 92
    '\'',   // 93
    '`',    // 94
    ',',    // 95
    '.',    // 96
    '/',    // 97

    // System keys → 0
    0,0,0,0,                       // 98–101

    // Windows keys
    0,0,0                          // 102–104
};

static const char key_ascii_map_shift_on[] = {
    0,      // KEY_NO_KEY = 0

    // Digits (top row)
    '!',    // 1  KEY_0
    '@',    // 2
    '#',    // 3
    '$',    // 4
    '%',    // 5
    '^',    // 6
    '&',    // 7
    '*',    // 8
    '(',    // 9
    ')',    // 10

    // Keypad digits
    '0',    // 11
    '1',    // 12
    '2',    // 13
    '3',    // 14
    '4',    // 15
    '5',    // 16
    '6',    // 17
    '7',    // 18
    '8',    // 19
    '9',    // 20

    // Letters (always lowercase here)
    'A',    // 21
    'B',    // 22
    'C',    // 23
    'D',    // 24
    'E',    // 25
    'F',    // 26
    'G',    // 27
    'H',    // 28
    'I',    // 29
    'J',    // 30
    'K',    // 31
    'L',    // 32
    'M',    // 33
    'N',    // 34
    'O',    // 35
    'P',    // 36
    'Q',    // 37
    'R',    // 38
    'S',    // 39
    'T',    // 40
    'U',    // 41
    'V',    // 42
    'W',    // 43
    'X',    // 44
    'Y',    // 45
    'Z',    // 46

    // F1–F12 → 0 (non-printable)
    0,0,0,0,0,0,0,0,0,0,0,0,   // 47–58

    // Control keys
    0,      // 59  ESC
    '\t',   // 60  TAB
    0,0,0,0,0,0,0,           // 61–67
    ' ',    // 68  SPACE
    '\n',   // 69  ENTER
    0,      // 70  BACKSPACE is handled separately

    // Navigation keys: all 0
    0,0,0,0,0,0,0,0,0,         // 71–80

    // Keypad ops
    '+',    // 81
    '-',    // 82
    '*',    // 83
    '/',    // 84
    '\n',   // 85  KP_ENTER
    '.',    // 86  KP_DOT

    // Symbols
    '_',    // 87 KEY_MINUS
    '+',    // 88
    '{',    // 89
    '}',    // 90
    '|',   // 91
    ':',    // 92
    '\"',   // 93
    '~',    // 94
    '<',    // 95
    '>',    // 96
    '?',    // 97

    // System keys → 0
    0,0,0,0,                       // 98–101

    // Windows keys
    0,0,0                          // 102–104
};

static uint8_t keyboard_state[KEY_COUNT];
static key_queue_t key_queue;  // key press queue
static uint8_t extended_key = 0; // 0 if keyboard got regular key, else 1 (for extended)
static uint8_t shift_on = 0;

char key_to_ascii(uint8_t key) {
    if (key >= KEY_COUNT)
        return 0;

    if (shift_on) return key_ascii_map_shift_on[key];
    else return key_ascii_map_shift_off[key];
}

void keyboard_handle_scancode(uint8_t scancode) {
    uint8_t key;
    uint8_t pressed;

    /* check if next scancode will be extended */
    if (scancode == KEY_EXTENDED) {
        extended_key = 1;
        return;
    }

    pressed = !(scancode & 0x80);

    if (extended_key == 1) {
        key = ex_code_to_key[scancode & 0x7f];
        extended_key = 0;
    } else {
        key = scancode_to_key_index[scancode & 0x7f];
    }


    /* update shift case */
    if (key == KEY_LSHIFT || key == KEY_RSHIFT) {
        shift_on = pressed;
    }
    
    /* check if there is a need to add the key to the tty putc queue */
    if ((pressed == KEY_PRESSED) && (keyboard_state[key] == KEY_RELEASED)) {
        put_key_press(key);
        if (key_to_ascii(key) != -1)
            putchar(key_to_ascii(key));
    }

    /* update keyboard state */
    keyboard_state[key] = pressed;
}

void keyboard_driver_init() {
    // set the key pressed queue
    key_queue.head = 0;
    key_queue.tail = 0;
    shift_on = 0;

    memset(key_queue.queue, 0, KEY_QUEUE_SIZE);
    
    register_interrupt_handler(33, keyboard_handler);
}

uint32_t keyboard_handler(cpu_status_t* regs){
    // Note: after testing we found that the current keyboard uses "Scan Code Set 1"
    uint8_t scancode = inb(0x60);

    keyboard_handle_scancode(scancode);

    return -ENO;
}

uint8_t is_key_pressed(uint8_t key){
    return keyboard_state[key] == KEY_PRESSED;
}

char get_asynchronized_char() {
    if (key_queue.head == key_queue.tail) return 0; // queue is empty, return 0

    char out = key_queue.queue[key_queue.tail];
    key_queue.tail = (key_queue.tail+1) % KEY_QUEUE_SIZE;

    return out;
}