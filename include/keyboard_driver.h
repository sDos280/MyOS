#ifndef KEY_MOUSE_H
#define KEY_MOUSE_H

#include "description_tables.h"

#define KEY_RELEASED 0
#define KEY_PRESSED  1

/*
 * Logical keyboard key indices
 * -----------------------------
 * These indices are used for your `keyboard_state[]` array.
 * Each index corresponds to a unique key *type*, not necessarily a unique scan code.
 * 
 * You can map multiple scan codes to the same logical key index (e.g. top-row '1' and keypad '1').
 */

//
// Digits (top row)
//
#define KEY_0           0
#define KEY_1           1
#define KEY_2           2
#define KEY_3           3
#define KEY_4           4
#define KEY_5           5
#define KEY_6           6
#define KEY_7           7
#define KEY_8           8
#define KEY_9           9

//
// Keypad digits
//
#define KEY_KP_0        10
#define KEY_KP_1        11
#define KEY_KP_2        12
#define KEY_KP_3        13
#define KEY_KP_4        14
#define KEY_KP_5        15
#define KEY_KP_6        16
#define KEY_KP_7        17
#define KEY_KP_8        18
#define KEY_KP_9        19

//
// Letters
//
#define KEY_A           20
#define KEY_B           21
#define KEY_C           22
#define KEY_D           23
#define KEY_E           24
#define KEY_F           25
#define KEY_G           26
#define KEY_H           27
#define KEY_I           28
#define KEY_J           29
#define KEY_K           30
#define KEY_L           31
#define KEY_M           32
#define KEY_N           33
#define KEY_O           34
#define KEY_P           35
#define KEY_Q           36
#define KEY_R           37
#define KEY_S           38
#define KEY_T           39
#define KEY_U           40
#define KEY_V           41
#define KEY_W           42
#define KEY_X           43
#define KEY_Y           44
#define KEY_Z           45

//
// Function keys
//
#define KEY_F1          46
#define KEY_F2          47
#define KEY_F3          48
#define KEY_F4          49
#define KEY_F5          50
#define KEY_F6          51
#define KEY_F7          52
#define KEY_F8          53
#define KEY_F9          54
#define KEY_F10         55
#define KEY_F11         56
#define KEY_F12         57

//
// Control and special keys
//
#define KEY_ESC         58
#define KEY_TAB         59
#define KEY_CAPSLOCK    60
#define KEY_LSHIFT      61
#define KEY_RSHIFT      62
#define KEY_LCTRL       63
#define KEY_RCTRL       64
#define KEY_LALT        65
#define KEY_RALT        66
#define KEY_SPACE       67
#define KEY_ENTER       68
#define KEY_BACKSPACE   69

//
// Navigation / editing keys
//
#define KEY_INSERT      70
#define KEY_DELETE      71
#define KEY_HOME        72
#define KEY_END         73
#define KEY_PAGEUP      74
#define KEY_PAGEDOWN    75
#define KEY_ARROW_UP    76
#define KEY_ARROW_DOWN  77
#define KEY_ARROW_LEFT  78
#define KEY_ARROW_RIGHT 79

//
// Keypad operations
//
#define KEY_KP_PLUS     80
#define KEY_KP_MINUS    81
#define KEY_KP_MUL      82
#define KEY_KP_DIV      83
#define KEY_KP_ENTER    84
#define KEY_KP_DOT      85

//
// Symbols (main keyboard)
//
#define KEY_MINUS       86
#define KEY_EQUAL       87
#define KEY_LBRACKET    88
#define KEY_RBRACKET    89
#define KEY_BACKSLASH   90
#define KEY_SEMICOLON   91
#define KEY_APOSTROPHE  92
#define KEY_GRAVE       93
#define KEY_COMMA       94
#define KEY_PERIOD      95
#define KEY_SLASH       96

//
// System keys
//
#define KEY_PRINT_SCREEN 97
#define KEY_SCROLL_LOCK  98
#define KEY_PAUSE_BREAK  99
#define KEY_NUM_LOCK     100

//
// Windows / Application keys
//
#define KEY_LSUPER       101
#define KEY_RSUPER       102
#define KEY_MENU         103

//
// Total count
//
#define KEY_COUNT        256

#define KEY_QUEUE_SIZE 50

typedef struct key_status_struct {
    uint8_t pressed;
} key_status_t;

typedef struct key_queue_struct {
    char queue[KEY_QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
} key_queue_t;

void initialize_keyboard_driver(); // initialize keyboard driver
void keyboard_handler(registers_t* regs);  // the keyboard interrupt handler
char get_asynchronized_char(); // get the next get asynchronized char (0 if there is no new char)

#endif // KEY_MOUSE_H