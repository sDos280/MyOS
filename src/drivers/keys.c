#include "drivers/keys.h"

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

char key_to_ascii(uint8_t key, uint8_t shift_on) {
    if (key >= KEY_COUNT)
        return 0;

    if (shift_on) return key_ascii_map_shift_on[key];
    else return key_ascii_map_shift_off[key];
}