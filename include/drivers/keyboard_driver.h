#ifndef KEYBOARD_DRIVER_H
#define KEYBOARD_DRIVER_H

#include "kernel/description_tables.h"

#define KEY_RELEASED 0
#define KEY_PRESSED  1

#define KEY_NO_KEY      0

//
// Digits (top row)
//
#define KEY_0           1
#define KEY_1           2
#define KEY_2           3
#define KEY_3           4
#define KEY_4           5
#define KEY_5           6
#define KEY_6           7
#define KEY_7           8
#define KEY_8           9
#define KEY_9           10

//
// Keypad digits
//
#define KEY_KP_0        11
#define KEY_KP_1        12
#define KEY_KP_2        13
#define KEY_KP_3        14
#define KEY_KP_4        15
#define KEY_KP_5        16
#define KEY_KP_6        17
#define KEY_KP_7        18
#define KEY_KP_8        19
#define KEY_KP_9        20

//
// Letters
//
#define KEY_A           21
#define KEY_B           22
#define KEY_C           23
#define KEY_D           24
#define KEY_E           25
#define KEY_F           26
#define KEY_G           27
#define KEY_H           28
#define KEY_I           29
#define KEY_J           30
#define KEY_K           31
#define KEY_L           32
#define KEY_M           33
#define KEY_N           34
#define KEY_O           35
#define KEY_P           36
#define KEY_Q           37
#define KEY_R           38
#define KEY_S           39
#define KEY_T           40
#define KEY_U           41
#define KEY_V           42
#define KEY_W           43
#define KEY_X           44
#define KEY_Y           45
#define KEY_Z           46

//
// Function keys
//
#define KEY_F1          47
#define KEY_F2          48
#define KEY_F3          49
#define KEY_F4          50
#define KEY_F5          51
#define KEY_F6          52
#define KEY_F7          53
#define KEY_F8          54
#define KEY_F9          55
#define KEY_F10         56
#define KEY_F11         57
#define KEY_F12         58

//
// Control and special keys
//
#define KEY_ESC         59
#define KEY_TAB         60
#define KEY_CAPSLOCK    61
#define KEY_LSHIFT      62
#define KEY_RSHIFT      63
#define KEY_LCTRL       64
#define KEY_RCTRL       65
#define KEY_LALT        66
#define KEY_RALT        67
#define KEY_SPACE       68
#define KEY_ENTER       69
#define KEY_BACKSPACE   70

//
// Navigation / editing keys
//
#define KEY_INSERT      71
#define KEY_DELETE      72
#define KEY_HOME        73
#define KEY_END         74
#define KEY_PAGEUP      75
#define KEY_PAGEDOWN    76
#define KEY_ARROW_UP    77
#define KEY_ARROW_DOWN  78
#define KEY_ARROW_LEFT  79
#define KEY_ARROW_RIGHT 80

//
// Keypad operations
//
#define KEY_KP_PLUS     81
#define KEY_KP_MINUS    82
#define KEY_KP_MUL      83
#define KEY_KP_DIV      84
#define KEY_KP_ENTER    85
#define KEY_KP_DOT      86

//
// Symbols (main keyboard)
//
#define KEY_MINUS       87
#define KEY_EQUAL       88
#define KEY_LBRACKET    89
#define KEY_RBRACKET    90
#define KEY_BACKSLASH   91
#define KEY_SEMICOLON   92
#define KEY_APOSTROPHE  93
#define KEY_GRAVE       94
#define KEY_COMMA       95
#define KEY_PERIOD      96
#define KEY_SLASH       97

//
// System keys
//
#define KEY_PRINT_SCREEN 98
#define KEY_SCROLL_LOCK  99
#define KEY_PAUSE_BREAK  100
#define KEY_NUM_LOCK     101

//
// Windows / Application keys
//
#define KEY_LSUPER       102
#define KEY_RSUPER       103
#define KEY_MENU         104

//
// Total count
//
#define KEY_COUNT        256

#define KEY_QUEUE_SIZE 50

#define KEY_EXTENDED 0xe0

typedef struct key_queue_struct {
    char queue[KEY_QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
} key_queue_t;

char key_to_ascii(uint8_t key);
void initialize_keyboard_driver(); // initialize keyboard driver
void keyboard_handler(registers_t* regs);  // the keyboard interrupt handler
uint8_t is_key_pressed(uint8_t key);
char get_asynchronized_char(); // get the next get asynchronized char (0 if there is no new char)

#endif // KEYBOARD_DRIVER_H