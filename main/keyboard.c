/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * Copyright 2017 Benjamin Aigner <beni@asterics-foundation.org>
 * 
 * Heavily based on Paul Stoffregens usb_api.cpp from Teensyduino
 * http://www.pjrc.com/teensy/teensyduino.html
 * Copyright (c) 2008 PJRC.COM, LLC
 * THANK YOU VERY MUCH FOR THIS EFFORT ON KEYBOARD + LAYOUTS!
 * 
 */
/** @file
 * @brief Keyboard layout/keycode helper functions
 * 
 * This module is basically a wrapper around Paul Stoffregen's
 * <b>keylayouts.h</b> file, which contains all keycodes & locales.
 * This file supports following tasks:<br>
 * * Get a keycode for one or more ASCII/Unicode bytes
 * * Get a HID country code for a locale
 * * Get a keycode for a key identifier (e.g., KEY_A)
 * * Get a key identifier for a keycode
 * * Fill/empty a keycode array for HID reports
 * 
 * This file is mostly used by task_kbd and task_commands modules.
 * 
 * This file is built around keylayouts.h, a header file for all
 * keyboard layouts from Paul Stoffregen.
 * To avoid modifying the header file, this file is playing some tricks
 * with preprocessor defines:
 * 
 * 1.) Include header "undefkeylayouts.h", which removes all previously set
 *     keycodes and different bit sets <br>
 * 
 * 2.) Define the currently used keyboard layout <br>
 * 
 * 3.) Include the "keylayouts.h" header <br>
 * 
 * 4.) Use all the bits, masks and keycodes <br>
 * 
 * 5.) Redo steps 1-4 for all different arrays for all keyboard layouts
 * 
 * @note Once again: Thank you very much Paul for these layouts!
 **/
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "keyboard.h"
#include "esp_log.h"
#define LOG_TAG "KB"

/** Helper macro to compare key identifiers, y must be of fixed size! */
#define COMP(x,y) ((memcmp(x,y,sizeof(y)-1) == 0) && (x[sizeof(y)-1] == '\0' || \
  x[sizeof(y)-1] == '\r' || x[sizeof(y)-1] == '\n' || x[sizeof(y)-1] == ' '))
/** Helper macro to save key identifier, x must be a static string! */
#define SAVE(x) if(buf_len > sizeof(x)) { memcpy(buffer,x,sizeof(x)); } else { return 2; } break



/** 
 * @brief Defines the bits and deadkeys itself for all available deadkeys
 * for US_INTERNATIONAL. All other layouts have an equal array.
 * If a layout does not have deadkeys at all, this array is of [2][1] dimension
 * and initialised with {0,0} */
const uint16_t deadkey_USINT[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_US_INTERNATIONAL
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
///@cond DONTINCLUDETHIS
const uint16_t deadkey_DE[2][3] = { 
  #include "undefkeylayouts.h"
  #define LAYOUT_GERMAN
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT}
};
const uint16_t deadkey_DEMAC[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_GERMAN_MAC
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_CAFR[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_CANADIAN_FRENCH
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, DIAERESIS_BITS, CEDILLA_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_DIAERESIS, DEADKEY_CEDILLA}
};
const uint16_t deadkey_CAINT[2][7] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_CANADIAN_MULTILINGUAL
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, DIAERESIS_BITS, CEDILLA_BITS, TILDE_BITS, RING_ABOVE_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_DIAERESIS, DEADKEY_CEDILLA, DEADKEY_TILDE, DEADKEY_RING_ABOVE}
};
const uint16_t deadkey_UK[2][1] = {{0},{0}};
const uint16_t deadkey_FI[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_FINNISH
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_FR[2][4] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_FRENCH
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_DK[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_DANISH
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_NW[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_NORWEGIAN
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_SW[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_SWEDISH
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_ES[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_SPANISH
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_PT[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_PORTUGUESE
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_IT[2][1] = {{0},{0}};

const uint16_t deadkey_PTBR[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_PORTUGUESE_BRAZILIAN
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_FRBE[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_FRENCH_BELGIAN
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_DESW[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_GERMAN_SWISS
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_FRSW[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_FRENCH_SWISS
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_ESLAT[2][4] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_SPANISH_LATIN_AMERICA
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_IR[2][2] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_IRISH
  #include "keylayouts.h"
  {ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS},
  {DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT}
};
const uint16_t deadkey_IC[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_ICELANDIC
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, RING_ABOVE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_RING_ABOVE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_TK[2][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_TURKISH
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, ACUTE_ACCENT_BITS, GRAVE_ACCENT_BITS, TILDE_BITS, DIAERESIS_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_ACUTE_ACCENT, DEADKEY_GRAVE_ACCENT, DEADKEY_TILDE, DEADKEY_DIAERESIS}
};
const uint16_t deadkey_CZ[2][8] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_CZECH
  #include "keylayouts.h"
  {/*CIRCUMFLEX_BITS,*/ DOUBLE_ACUTE_BITS, CEDILLA_BITS, DEGREE_SIGN_BITS, CARON_BITS, ACUTE_ACCENT_BITS, BREVE_BITS, /*GRAVE_ACCENT_BITS,*/ DOT_ABOVE_BITS, /*DIAERESIS_BITS, */ OGONEK_BITS},
  {/*DEADKEY_CIRCUMFLEX,*/ DEADKEY_DOUBLE_ACUTE, DEADKEY_CEDILLA, DEADKEY_DEGREE_SIGN, DEADKEY_CARON, DEADKEY_ACUTE_ACCENT, DEADKEY_BREVE, /*DEADKEY_GRAVE_ACCENT,*/ DEADKEY_DOT_ABOVE, /*DEADKEY_DIAERESIS, */ DEADKEY_OGONEK} 
};
const uint16_t deadkey_SR[2][10] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_SERBIAN_LATIN_ONLY
  #include "keylayouts.h"
  {CIRCUMFLEX_BITS, DOUBLE_ACUTE_BITS, CEDILLA_BITS, DEGREE_SIGN_BITS, CARON_BITS, ACUTE_ACCENT_BITS, BREVE_BITS, DOT_ABOVE_BITS, DIAERESIS_BITS, OGONEK_BITS},
  {DEADKEY_CIRCUMFLEX, DEADKEY_DOUBLE_ACUTE, DEADKEY_CEDILLA, DEADKEY_DEGREE_SIGN, DEADKEY_CARON, DEADKEY_ACUTE_ACCENT, DEADKEY_BREVE, DEADKEY_DOT_ABOVE, DEADKEY_DIAERESIS, DEADKEY_OGONEK} 
};
///@endcond

/** 
 * @brief Defines all masks necessary for either getting modifiers or
 * the keycode. */
const uint16_t keycodes_masks[][5] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_US_ENGLISH
  #include "keylayouts.h"
  { SHIFT_MASK, 0, 0, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_US_INTERNATIONAL
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  ///@cond DONTINCLUDETHIS
  #include "undefkeylayouts.h"
  #define LAYOUT_GERMAN
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_GERMAN_MAC
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_CANADIAN_FRENCH
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_CANADIAN_MULTILINGUAL
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, RCTRL_MASK},
  #include "undefkeylayouts.h"
  #define LAYOUT_UNITED_KINGDOM
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, 0, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_FINNISH
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_FRENCH
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_DANISH
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_NORWEGIAN
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_SWEDISH
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_SPANISH
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_PORTUGUESE
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_ITALIAN
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, 0, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_PORTUGUESE_BRAZILIAN
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_FRENCH_BELGIAN
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_GERMAN_SWISS
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_FRENCH_SWISS
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_SPANISH_LATIN_AMERICA
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_IRISH
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_ICELANDIC
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_TURKISH
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_CZECH
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0},
  #include "undefkeylayouts.h"
  #define LAYOUT_SERBIAN_LATIN_ONLY
  #include "keylayouts.h"
  { SHIFT_MASK, ALTGR_MASK, DEADKEYS_MASK, KEYCODE_MASK, 0}
  ///@endcond
};


/** @brief Is this keycode a modifier?
 * 
 * This method is used to determine if a keycode is a modifier key
 * (without any other keys)
 * @param keycode Keycode to be tested
 * @return 0 if a normal keycode, 1 if a modifier key
 * */
uint8_t keycode_is_modifier(uint16_t keycode)
{
  if((keycode & 0xFF00) == 0xE000)
  {
    return 1;
  } else {
    return 0;
  }
}

/**
 * @brief  Array of pointers to a deadkey bits array for each locale.
 * 
 * We are using offset 0 from the deadkey_XXX array, because it contains
 * the bitmasks.
 * 
 * @see deadkey_USINT (or any other language)
 * @warning Locale offset is different here, because there is no
 * LAYOUT_US_ENGLISH (no deadkeys at all)
 * */
const uint16_t* keycodes_deadkey_bits[] = {
  deadkey_USINT[0],
  deadkey_DE[0],
  deadkey_DEMAC[0],
  deadkey_CAFR[0],
  deadkey_CAINT[0],
  deadkey_UK[0],
  deadkey_FI[0],
  deadkey_FR[0],
  deadkey_DK[0],
  deadkey_NW[0],
  deadkey_SW[0],
  deadkey_ES[0],
  deadkey_PT[0],
  deadkey_IT[0],
  deadkey_PTBR[0],
  deadkey_FRBE[0],
  deadkey_DESW[0],
  deadkey_FRSW[0],
  deadkey_ESLAT[0],
  deadkey_IR[0],
  deadkey_IC[0],
  deadkey_TK[0],
  deadkey_CZ[0],
  deadkey_SR[0]
};

/**
 * @brief  Array of pointers to a deadkey keycodes array for each locale.
 * 
 * We are using offset 1 from the deadkey_XXX array, because it contains
 * the keycodes.
 * 
 * @see deadkey_USINT (or any other language)
 * @warning Locale offset is different here, because there is no
 * LAYOUT_US_ENGLISH (no deadkeys at all)
 * */
const uint16_t* keycodes_deadkey[] = {
  deadkey_USINT[1],
  deadkey_DE[1],
  deadkey_DEMAC[1],
  deadkey_CAFR[1],
  deadkey_CAINT[1],
  deadkey_UK[1],
  deadkey_FI[1],
  deadkey_FR[1],
  deadkey_DK[1],
  deadkey_NW[1],
  deadkey_SW[1],
  deadkey_ES[1],
  deadkey_PT[1],
  deadkey_IT[1],
  deadkey_PTBR[1],
  deadkey_FRBE[1],
  deadkey_DESW[1],
  deadkey_FRSW[1],
  deadkey_ESLAT[1],
  deadkey_IR[1],
  deadkey_IC[1],
  deadkey_TK[1],
  deadkey_CZ[1],
  deadkey_SR[1]
};


#if 0
/** TODO: add extra unicodes; most layouts just use UNICODE_EXTRA00
 * which is mostly 0x20AC (Euro Sign). */
const uint16_t keycodes_unicode_extra[][11] = {
    #include "undefkeylayouts.h"
    #define LAYOUT_US_ENGLISH
    #include "keylayouts.h"
    {
    UNICODE_EXTRA00,KEYCODE_EXTRA00,UNICODE_EXTRA01,KEYCODE_EXTRA01,
    UNICODE_EXTRA02,KEYCODE_EXTRA02,UNICODE_EXTRA03,KEYCODE_EXTRA03,
    UNICODE_EXTRA04,KEYCODE_EXTRA04,UNICODE_EXTRA05,KEYCODE_EXTRA05,
    UNICODE_EXTRA06,KEYCODE_EXTRA06,UNICODE_EXTRA07,KEYCODE_EXTRA07,
    UNICODE_EXTRA08,KEYCODE_EXTRA08,UNICODE_EXTRA09,KEYCODE_EXTRA09,
    UNICODE_EXTRA0A,KEYCODE_EXTRA0A
    }
  
};
#endif


/**
 * @brief Array of all keycodes for ASCII symbols.
 * 
 * The array offset is on the one hand the locale as it is defined
 * in keyboard_layouts and on the other hand the ASCII number - 0x20.
 * --> ASCII 0x22 character ('2') is for US_ENGLISH on position [0][2]
 * 
 * @see keyboard_layouts
 * @warning Locale offset is different to the deadkey arrays, we have
 * LAYOUT_US_ENGLISH here.
 * */
const uint16_t keycodes_ascii[][96] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_US_ENGLISH
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_US_INTERNATIONAL
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  ///@cond DONTINCLUDETHIS
  #include "undefkeylayouts.h"
  #define LAYOUT_GERMAN
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_GERMAN_MAC
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_CANADIAN_FRENCH
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_CANADIAN_MULTILINGUAL
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_UNITED_KINGDOM
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_FINNISH
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_FRENCH
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_DANISH
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_NORWEGIAN
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_SWEDISH
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_SPANISH
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_PORTUGUESE
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_ITALIAN
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_PORTUGUESE_BRAZILIAN
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_FRENCH_BELGIAN
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_GERMAN_SWISS
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_FRENCH_SWISS
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_SPANISH_LATIN_AMERICA
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_IRISH
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_ICELANDIC
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_TURKISH
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_CZECH
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_SERBIAN_LATIN_ONLY
  #include "keylayouts.h"
  {
    ASCII_20, ASCII_21, ASCII_22, ASCII_23,
    ASCII_24, ASCII_25, ASCII_26, ASCII_27,
    ASCII_28, ASCII_29, ASCII_2A, ASCII_2B,
    ASCII_2C, ASCII_2D, ASCII_2E, ASCII_2F,
    ASCII_30, ASCII_31, ASCII_32, ASCII_33,
    ASCII_34, ASCII_35, ASCII_36, ASCII_37,
    ASCII_38, ASCII_39, ASCII_3A, ASCII_3B,
    ASCII_3C, ASCII_3D, ASCII_3E, ASCII_3F,
    ASCII_40, ASCII_41, ASCII_42, ASCII_43,
    ASCII_44, ASCII_45, ASCII_46, ASCII_47,
    ASCII_48, ASCII_49, ASCII_4A, ASCII_4B,
    ASCII_4C, ASCII_4D, ASCII_4E, ASCII_4F,
    ASCII_50, ASCII_51, ASCII_52, ASCII_53,
    ASCII_54, ASCII_55, ASCII_56, ASCII_57,
    ASCII_58, ASCII_59, ASCII_5A, ASCII_5B,
    ASCII_5C, ASCII_5D, ASCII_5E, ASCII_5F,
    ASCII_60, ASCII_61, ASCII_62, ASCII_63,
    ASCII_64, ASCII_65, ASCII_66, ASCII_67,
    ASCII_68, ASCII_69, ASCII_6A, ASCII_6B,
    ASCII_6C, ASCII_6D, ASCII_6E, ASCII_6F,
    ASCII_70, ASCII_71, ASCII_72, ASCII_73,
    ASCII_74, ASCII_75, ASCII_76, ASCII_77,
    ASCII_78, ASCII_79, ASCII_7A, ASCII_7B,
    ASCII_7C, ASCII_7D, ASCII_7E, ASCII_7F
  }
  ///@endcond
};

/**
 * @brief  Array of all keycodes for ISO8859 (unicode) symbols/code points.
 * 
 * The array offset is on the one hand the locale as it is defined
 * in keyboard_layouts and on the other hand the unicode number - 0xA0.
 * --> unicode 0xA2 character is for US_INTERNATIONAL on position [0][2]
 * 
 * @see keyboard_layouts
 * @warning Locale offset is different to the ASCII array, we don't have
 * LAYOUT_US_ENGLISH here.
 * */
const uint16_t keycodes_iso_8859_1[][96] = {
  #include "undefkeylayouts.h"
  #define LAYOUT_US_INTERNATIONAL
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  ///@cond DONTINCLUDETHIS
  #include "undefkeylayouts.h"
  #define LAYOUT_GERMAN
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_GERMAN_MAC
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_CANADIAN_FRENCH
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_CANADIAN_MULTILINGUAL
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_UNITED_KINGDOM
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_FINNISH
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_FRENCH
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_DANISH
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_NORWEGIAN
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_SWEDISH
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_SPANISH
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_PORTUGUESE
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_ITALIAN
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_PORTUGUESE_BRAZILIAN
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_FRENCH_BELGIAN
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_GERMAN_SWISS
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_FRENCH_SWISS
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_SPANISH_LATIN_AMERICA
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_IRISH
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_ICELANDIC
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_TURKISH
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_CZECH
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  },
  #include "undefkeylayouts.h"
  #define LAYOUT_SERBIAN_LATIN_ONLY
  #include "keylayouts.h"
  {
  ISO_8859_1_A0, ISO_8859_1_A1, ISO_8859_1_A2, ISO_8859_1_A3,
  ISO_8859_1_A4, ISO_8859_1_A5, ISO_8859_1_A6, ISO_8859_1_A7,
  ISO_8859_1_A8, ISO_8859_1_A9, ISO_8859_1_AA, ISO_8859_1_AB,
  ISO_8859_1_AC, ISO_8859_1_AD, ISO_8859_1_AE, ISO_8859_1_AF,
  ISO_8859_1_B0, ISO_8859_1_B1, ISO_8859_1_B2, ISO_8859_1_B3,
  ISO_8859_1_B4, ISO_8859_1_B5, ISO_8859_1_B6, ISO_8859_1_B7,
  ISO_8859_1_B8, ISO_8859_1_B9, ISO_8859_1_BA, ISO_8859_1_BB,
  ISO_8859_1_BC, ISO_8859_1_BD, ISO_8859_1_BE, ISO_8859_1_BF,
  ISO_8859_1_C0, ISO_8859_1_C1, ISO_8859_1_C2, ISO_8859_1_C3,
  ISO_8859_1_C4, ISO_8859_1_C5, ISO_8859_1_C6, ISO_8859_1_C7,
  ISO_8859_1_C8, ISO_8859_1_C9, ISO_8859_1_CA, ISO_8859_1_CB,
  ISO_8859_1_CC, ISO_8859_1_CD, ISO_8859_1_CE, ISO_8859_1_CF,
  ISO_8859_1_D0, ISO_8859_1_D1, ISO_8859_1_D2, ISO_8859_1_D3,
  ISO_8859_1_D4, ISO_8859_1_D5, ISO_8859_1_D6, ISO_8859_1_D7,
  ISO_8859_1_D8, ISO_8859_1_D9, ISO_8859_1_DA, ISO_8859_1_DB,
  ISO_8859_1_DC, ISO_8859_1_DD, ISO_8859_1_DE, ISO_8859_1_DF,
  ISO_8859_1_E0, ISO_8859_1_E1, ISO_8859_1_E2, ISO_8859_1_E3,
  ISO_8859_1_E4, ISO_8859_1_E5, ISO_8859_1_E6, ISO_8859_1_E7,
  ISO_8859_1_E8, ISO_8859_1_E9, ISO_8859_1_EA, ISO_8859_1_EB,
  ISO_8859_1_EC, ISO_8859_1_ED, ISO_8859_1_EE, ISO_8859_1_EF,
  ISO_8859_1_F0, ISO_8859_1_F1, ISO_8859_1_F2, ISO_8859_1_F3,
  ISO_8859_1_F4, ISO_8859_1_F5, ISO_8859_1_F6, ISO_8859_1_F7,
  ISO_8859_1_F8, ISO_8859_1_F9, ISO_8859_1_FA, ISO_8859_1_FB,
  ISO_8859_1_FC, ISO_8859_1_FD, ISO_8859_1_FE, ISO_8859_1_FF
  }
  ///@endcond
};

/** @brief Parse a decoded code point to a keycode, step 2
 * 
 * This method parses a fully assembled code point to a
 * keycode. To get a fully assembled cpoint, use the method parse_for_keycode.
 * 
 * @see parse_for_keycode
 * @see keycodes_masks
 * @see keycodes_ascii
 * @see keycodes_iso_8859_1
 * @param cpoint Fully assembled ASCII or ISO8859-1 code point
 * @param locale Currently used keyboard layout
 * @return 0 if no keycode was found (invalid cpoint), the keycode otherwise
 */
uint16_t unicode_to_keycode(uint16_t cpoint, uint8_t locale)
{
	// Unicode code points beyond U+FFFF are not supported
	// technically this input should probably be called UCS-2
	if (cpoint < 32) {
		if (cpoint == 10) 
    {
      ESP_LOGI(LOG_TAG,"cpoint %d, locale %d, mask %d, key %d",cpoint,locale,keycodes_masks[locale][3],KEY_ENTER);
      return KEY_ENTER & keycodes_masks[locale][3];
    }
		if (cpoint == 11) 
    {
      ESP_LOGI(LOG_TAG,"cpoint %d, locale %d, mask %d, key %d",cpoint,locale,keycodes_masks[locale][3],KEY_TAB);
      return KEY_TAB & keycodes_masks[locale][3];
    }
		return 0;
	}
	if (cpoint < 128) 
  {
    ESP_LOGI(LOG_TAG,"ASCII lookup");
    ESP_LOGI(LOG_TAG,"cpoint %d, locale %d, mask --, key %d",cpoint,locale,keycodes_ascii[locale][cpoint - 0x20]);
    return keycodes_ascii[locale][cpoint - 0x20];
  }
	if (cpoint <= 0xA0) 
  {
    ESP_LOGE(LOG_TAG,"unkown cpoint");
    return 0;
  }
	if (cpoint < 0x100)
  {
    if(locale == LAYOUT_US_ENGLISH) 
    {
      ESP_LOGE(LOG_TAG,"locale is LAYOUT_US_ENGLISH, no unicode available");
      return 0;
    } else {
      ESP_LOGI(LOG_TAG,"cpoint %d, locale %d, mask --, key %d",cpoint,locale,keycodes_iso_8859_1[locale-1][cpoint-0xA0]);
      return keycodes_iso_8859_1[locale-1][cpoint-0xA0];
    }
  }
	//TODO: do UNICODE_EXTRA characters here....
  ESP_LOGE(LOG_TAG,"nothing applies in unicode_to_keycode");
	return 0;
}


/** @brief Parse a keycode for deadkey input, step 3
 * 
 * This method parses a keycode for a possible deadkey
 * sequence. If the parsed keycode needs a deadkey press, the 
 * corresponding keycode is returned. If no deadkey is required, 0 is returned.
 * To get a keycode, use the method unicode_to_keycode.
 * 
 * @see unicode_to_keycode
 * @see keycodes_masks
 * @param keycode Keycode which might need a deadkey pressed
 * @param locale Currently used keyboard layout
 * @return 0 if no deadkey needs to be pressed, the deadkey keycode otherwise
 */
uint16_t deadkey_to_keycode(uint16_t keycode, uint8_t locale)
{
	keycode &= keycodes_masks[locale][2];
	if (keycode == 0) return 0;
  ESP_LOGI(LOG_TAG,"deadkeys: applying mask 0x%X, result: %d", keycodes_masks[locale][2], keycode);
  for(uint8_t i = 0; i<sizeof(keycodes_deadkey_bits[locale]); i++)
  {
    if(keycode == keycodes_deadkey_bits[locale][i])
    {
      ESP_LOGI(LOG_TAG,"deadkey found, index: %d, deadkey: %d",i,keycodes_deadkey[locale][i]);
      return keycodes_deadkey[locale][i];
    }
  }
  ESP_LOGI(LOG_TAG,"no deadkey");
	return 0;
}

/** @brief Mask the keycode to get the HID keycode, step 4
 * 
 * This method masks out all modifier bits and returns the direct
 * HID keycode, which can be used in HID reports.
 * 
 * @param keycode Keycode from other parsing methods
 * @return 8-bit keycode for HID
 **/
uint8_t keycode_to_key(uint16_t keycode)
{
	uint8_t key = keycode & 0x7F;
	if (key == KEY_NON_US_100) key = 100;
	return key;
}

/** @brief Mask the keycode to get the modifiers, step 5
 * 
 * This method masks out all keycode bits and returns the direct
 * HID modifier byte, which can be used in HID reports.
 * 
 * @param keycode Keycode from other parsing methods
 * @param locale Currently used keyboard layout
 * @return 8-bit keycode for HID
 * 
 * @todo Add remaining modifier keys (according to keylayouts.h, for example KEY_GUI)
 **/
uint8_t keycode_to_modifier(uint16_t keycode, uint8_t locale)
{
	uint8_t modifier=0;
	if (keycode & keycodes_masks[locale][0]) modifier |= MODIFIERKEY_SHIFT;
	if (keycode & keycodes_masks[locale][1]) modifier |= MODIFIERKEY_RIGHT_ALT;
	if (keycode & keycodes_masks[locale][4]) modifier |= MODIFIERKEY_RIGHT_CTRL;
  if(modifier) ESP_LOGI(LOG_TAG,"found modifiers: %X",modifier);
	return modifier;
}


/** @brief Parse an incoming byte for a keycode
 * 
 * This method parses one incoming byte for the given locale.
 * It returns 0 if there is no keycode or another byte is needed (Unicode input).
 * If a modifier is needed the given modifier byte is updated.
 * 
 * @see keyboard_layouts
 * @param inputdata Input byte to parse for a keycode
 * @param locale Use this keyboard layout
 * @param keycode_modifier If a modifier is needed, it will be written here
 * @param deadkey_first_keycode If a deadkey stroke is needed, it will be written here
 * @return 0 if another byte is needed or no keycode is found; the keycode otherwise
 * 
 * */
uint8_t parse_for_keycode(uint8_t inputdata, uint8_t locale, uint8_t *keycode_modifier, uint8_t *deadkey_first_keycode)
{
  static uint8_t utf8_state = 0;
  static uint16_t unicode_wchar = 0;
  static uint16_t keycode = 0;
  static uint8_t deadkey = 0;
  
  //avoid accessing arrays out of bound
  if(locale >= LAYOUT_MAX) return 0;
  
  if (inputdata < 0x80) {
		// single byte encoded, 0x00 to 0x7F
		utf8_state = 0;
    keycode = unicode_to_keycode(inputdata, locale);
    deadkey = deadkey_to_keycode(keycode,locale);
    *keycode_modifier = keycode_to_modifier(keycode,locale);
    if(deadkey) *deadkey_first_keycode = keycode_to_key(deadkey);
    else *deadkey_first_keycode = 0;
		return keycode_to_key(keycode & keycodes_masks[locale][3]);
	} else if (inputdata < 0xC0) {
		// 2nd, 3rd or 4th byte, 0x80 to 0xBF
		inputdata &= 0x3F;
		if (utf8_state == 1) {
			utf8_state = 0;
      keycode = unicode_to_keycode(unicode_wchar | inputdata, locale);
      deadkey = deadkey_to_keycode(keycode,locale);
      *keycode_modifier = keycode_to_modifier(keycode,locale);
      if(deadkey) *deadkey_first_keycode = keycode_to_key(deadkey);
      else *deadkey_first_keycode = 0;
      return keycode_to_key(keycode & keycodes_masks[locale][3]);
		} else if (utf8_state == 2) {
			unicode_wchar |= ((uint16_t)inputdata << 6);
			utf8_state = 1;
		}
	} else if (inputdata < 0xE0) {
		// begin 2 byte sequence, 0xC2 to 0xDF
		// or illegal 2 byte sequence, 0xC0 to 0xC1
		unicode_wchar = (uint16_t)(inputdata & 0x1F) << 6;
		utf8_state = 1;
	} else if (inputdata < 0xF0) {
		// begin 3 byte sequence, 0xE0 to 0xEF
		unicode_wchar = (uint16_t)(inputdata & 0x0F) << 12;
		utf8_state = 2;
	} else {
		// begin 4 byte sequence (not supported), 0xF0 to 0xF4
		// or illegal, 0xF5 to 0xFF
		utf8_state = 255;
	}
	return 0;
}

/** @brief Remove a keycode from the given HID keycode array.
 * 
 * @note The size of the keycode_arr parameter MUST be 6
 * @param keycode Keycode to be removed
 * @param keycode_arr Keycode to remove this keycode from
 * @return 0 if the keycode was removed, 1 if the keycode was not in the array
 * */
uint8_t remove_keycode(uint8_t keycode,uint8_t *keycode_arr)
{
  uint8_t ret = 1;
  if(keycode == 0) return 0;
  //source: Arduino core Keyboard.cpp
  for (uint8_t i=0; i<6; i++) {
		if (keycode_arr[i] == keycode) 
    {
      keycode_arr[i] = 0;
      ret = 0;
    }
	}
  return ret;
}

/** @brief Add a keycode to the given HID keycode array.
 * 
 * @note The size of the keycode_arr parameter MUST be 6
 * @param keycode Keycode to be added
 * @param keycode_arr Keycode to add this keycode to
 * @return 0 if the keycode was added, 1 if the keycode was already in the array, 2 if there was no space
 * */
uint8_t add_keycode(uint8_t keycode,uint8_t *keycode_arr)
{
  uint8_t i;
  if(keycode == 0) return 0;
  
  //source: Arduino core Keyboard.cpp
  // Add k to the key array only if it's not already present
	// and if there is an empty slot.
	if (keycode_arr[0] != keycode && keycode_arr[1] != keycode && 
		keycode_arr[2] != keycode && keycode_arr[3] != keycode &&
		keycode_arr[4] != keycode && keycode_arr[5] != keycode) {
		
		for (i=0; i<6; i++) {
			if (keycode_arr[i] == 0x00) {
				keycode_arr[i] = keycode;
				return 0;
			}
		}
		if (i == 6) {
			return 2;
		}	
	}
  return 1;
}

/** @brief Test if key is in given keycode array
 * @note The size of the keycode_arr parameter MUST be 6
 * @param keycode Keycode to be tested
 * @param keycode_arr Array to test
 * @return 0 if the keycode is not in array, 1 if the keycode is in the array
 */
uint8_t is_in_keycode_arr(uint8_t keycode,uint8_t *keycode_arr)
{
	if (keycode_arr[0] != keycode && keycode_arr[1] != keycode &&
			keycode_arr[2] != keycode && keycode_arr[3] != keycode &&
			keycode_arr[4] != keycode && keycode_arr[5] != keycode)
	{
		return 0;
	} else {
		return 1;
	}
}

/** @brief Parse a key identifier to a keycode
 * 
 * This method is used to parse a key identifier (e.g., KEY_A)
 * to a keycode which is used for a task_keyboard config.
 * 
 * @warning If you use key identifiers, no keyboard locale is taken into
 * account!
 * 
 * @param keyidentifier Key identifier string
 * @return Keycode if found, 0 otherwise
 * 
 * @see parseKeycodeToIdentifier
 * */
uint16_t parseIdentifierToKeycode(char* keyidentifier)
{    
  if(COMP(keyidentifier, "KEY_1")) return KEY_1;
  if(COMP(keyidentifier, "KEY_2")) return KEY_2;
  if(COMP(keyidentifier, "KEY_3")) return KEY_3;
  if(COMP(keyidentifier, "KEY_4")) return KEY_4;
  if(COMP(keyidentifier, "KEY_5")) return KEY_5;
  if(COMP(keyidentifier, "KEY_6")) return KEY_6;
  if(COMP(keyidentifier, "KEY_7")) return KEY_7;
  if(COMP(keyidentifier, "KEY_8")) return KEY_8;
  if(COMP(keyidentifier, "KEY_9")) return KEY_9;
  if(COMP(keyidentifier, "KEY_0")) return KEY_0;
  
  if(COMP(keyidentifier, "KEY_F1")) return KEY_F1;
  if(COMP(keyidentifier, "KEY_F2")) return KEY_F2;
  if(COMP(keyidentifier, "KEY_F3")) return KEY_F3;
  if(COMP(keyidentifier, "KEY_F4")) return KEY_F4;
  if(COMP(keyidentifier, "KEY_F5")) return KEY_F5;
  if(COMP(keyidentifier, "KEY_F6")) return KEY_F6;
  if(COMP(keyidentifier, "KEY_F7")) return KEY_F7;
  if(COMP(keyidentifier, "KEY_F8")) return KEY_F8;
  if(COMP(keyidentifier, "KEY_F9")) return KEY_F9;
  if(COMP(keyidentifier, "KEY_F10")) return KEY_F10;
  if(COMP(keyidentifier, "KEY_F11")) return KEY_F11;
  if(COMP(keyidentifier, "KEY_F12")) return KEY_F12;
  if(COMP(keyidentifier, "KEY_F13")) return KEY_F13;
  if(COMP(keyidentifier, "KEY_F14")) return KEY_F14;
  if(COMP(keyidentifier, "KEY_F15")) return KEY_F15;
  if(COMP(keyidentifier, "KEY_F16")) return KEY_F16;
  if(COMP(keyidentifier, "KEY_F17")) return KEY_F17;
  if(COMP(keyidentifier, "KEY_F18")) return KEY_F18;
  if(COMP(keyidentifier, "KEY_F19")) return KEY_F19;
  if(COMP(keyidentifier, "KEY_F20")) return KEY_F20;
  if(COMP(keyidentifier, "KEY_F21")) return KEY_F21;
  if(COMP(keyidentifier, "KEY_F22")) return KEY_F22;
  if(COMP(keyidentifier, "KEY_F23")) return KEY_F23;
  if(COMP(keyidentifier, "KEY_F24")) return KEY_F24;
  
  if(COMP(keyidentifier, "KEY_RIGHT")) return KEY_RIGHT;
  if(COMP(keyidentifier, "KEY_LEFT")) return KEY_LEFT;
  if(COMP(keyidentifier, "KEY_DOWN")) return KEY_DOWN;
  if(COMP(keyidentifier, "KEY_UP")) return KEY_UP;
  
  if(COMP(keyidentifier, "KEY_ENTER")) return KEY_ENTER;
  if(COMP(keyidentifier, "KEY_ESC")) return KEY_ESC;
  if(COMP(keyidentifier, "KEY_BACKSPACE")) return KEY_BACKSPACE;
  if(COMP(keyidentifier, "KEY_TAB")) return KEY_TAB;
  if(COMP(keyidentifier, "KEY_HOME")) return KEY_HOME;
  if(COMP(keyidentifier, "KEY_PAGE_UP")) return KEY_PAGE_UP;
  if(COMP(keyidentifier, "KEY_PAGE_DOWN")) return KEY_PAGE_DOWN;
  if(COMP(keyidentifier, "KEY_DELETE")) return KEY_DELETE;
  if(COMP(keyidentifier, "KEY_INSERT")) return KEY_INSERT;
  if(COMP(keyidentifier, "KEY_END")) return KEY_END;
  if(COMP(keyidentifier, "KEY_NUM_LOCK")) return KEY_NUM_LOCK;
  if(COMP(keyidentifier, "KEY_SCROLL_LOCK")) return KEY_SCROLL_LOCK;
  if(COMP(keyidentifier, "KEY_SPACE")) return KEY_SPACE;
  if(COMP(keyidentifier, "KEY_CAPS_LOCK")) return KEY_CAPS_LOCK;
  if(COMP(keyidentifier, "KEY_PAUSE")) return KEY_PAUSE;
  if(COMP(keyidentifier, "KEY_SHIFT")) return MODIFIERKEY_SHIFT;
  if(COMP(keyidentifier, "KEY_CTRL")) return MODIFIERKEY_CTRL;
  if(COMP(keyidentifier, "KEY_ALT")) return MODIFIERKEY_ALT;
  if(COMP(keyidentifier, "KEY_RIGHT_ALT")) return KEY_RIGHT_ALT;
  if(COMP(keyidentifier, "KEY_GUI")) return MODIFIERKEY_GUI;
  if(COMP(keyidentifier, "KEY_RIGHT_GUI")) return KEY_RIGHT_GUI;
  
  if(COMP(keyidentifier, "KEY_MEDIA_POWER")) return KEY_MEDIA_POWER;
  if(COMP(keyidentifier, "KEY_MEDIA_RESET")) return KEY_MEDIA_RESET;
  if(COMP(keyidentifier, "KEY_MEDIA_SLEEP")) return KEY_MEDIA_SLEEP;
  if(COMP(keyidentifier, "KEY_MEDIA_MENU")) return KEY_MEDIA_MENU;
  if(COMP(keyidentifier, "KEY_MEDIA_SELECTION")) return KEY_MEDIA_SELECTION;
  if(COMP(keyidentifier, "KEY_MEDIA_ASSIGN_SEL")) return KEY_MEDIA_ASSIGN_SEL;
  if(COMP(keyidentifier, "KEY_MEDIA_MODE_STEP")) return KEY_MEDIA_MODE_STEP;
  if(COMP(keyidentifier, "KEY_MEDIA_RECALL_LAST")) return KEY_MEDIA_RECALL_LAST;
  if(COMP(keyidentifier, "KEY_MEDIA_QUIT")) return KEY_MEDIA_QUIT;
  if(COMP(keyidentifier, "KEY_MEDIA_HELP")) return KEY_MEDIA_HELP;
  if(COMP(keyidentifier, "KEY_MEDIA_CHANNEL_UP")) return KEY_MEDIA_CHANNEL_UP;
  if(COMP(keyidentifier, "KEY_MEDIA_CHANNEL_DOWN")) return KEY_MEDIA_CHANNEL_DOWN;
  if(COMP(keyidentifier, "KEY_MEDIA_SELECT_DISC")) return KEY_MEDIA_SELECT_DISC;
  if(COMP(keyidentifier, "KEY_MEDIA_ENTER_DISC")) return KEY_MEDIA_ENTER_DISC;
  if(COMP(keyidentifier, "KEY_MEDIA_REPEAT")) return KEY_MEDIA_REPEAT;
  if(COMP(keyidentifier, "KEY_MEDIA_VOLUME")) return KEY_MEDIA_VOLUME;
  if(COMP(keyidentifier, "KEY_MEDIA_BALANCE")) return KEY_MEDIA_BALANCE;
  if(COMP(keyidentifier, "KEY_MEDIA_BASS")) return KEY_MEDIA_BASS;
  
  if(COMP(keyidentifier, "KEY_MEDIA_PLAY")) return KEY_MEDIA_PLAY;
  if(COMP(keyidentifier, "KEY_MEDIA_PAUSE")) return KEY_MEDIA_PAUSE;
  if(COMP(keyidentifier, "KEY_MEDIA_RECORD")) return KEY_MEDIA_RECORD;
  if(COMP(keyidentifier, "KEY_MEDIA_FAST_FORWARD")) return KEY_MEDIA_FAST_FORWARD;
  if(COMP(keyidentifier, "KEY_MEDIA_REWIND")) return KEY_MEDIA_REWIND;
  if(COMP(keyidentifier, "KEY_MEDIA_NEXT_TRACK")) return KEY_MEDIA_NEXT_TRACK;
  if(COMP(keyidentifier, "KEY_MEDIA_PREV_TRACK")) return KEY_MEDIA_PREV_TRACK;
  if(COMP(keyidentifier, "KEY_MEDIA_STOP")) return KEY_MEDIA_STOP;
  if(COMP(keyidentifier, "KEY_MEDIA_EJECT")) return KEY_MEDIA_EJECT;
  if(COMP(keyidentifier, "KEY_MEDIA_RANDOM_PLAY")) return KEY_MEDIA_RANDOM_PLAY;
  if(COMP(keyidentifier, "KEY_MEDIA_STOP_EJECT")) return KEY_MEDIA_STOP_EJECT;
  if(COMP(keyidentifier, "KEY_MEDIA_PLAY_PAUSE")) return KEY_MEDIA_PLAY_PAUSE;
  if(COMP(keyidentifier, "KEY_MEDIA_PLAY_SKIP")) return KEY_MEDIA_PLAY_SKIP;
  if(COMP(keyidentifier, "KEY_MEDIA_MUTE")) return KEY_MEDIA_MUTE;
  if(COMP(keyidentifier, "KEY_MEDIA_VOLUME_INC")) return KEY_MEDIA_VOLUME_INC;
  if(COMP(keyidentifier, "KEY_MEDIA_VOLUME_DEC")) return KEY_MEDIA_VOLUME_DEC;
  
  if(COMP(keyidentifier, "KEY_SYSTEM_POWER_DOWN")) return KEY_SYSTEM_POWER_DOWN;
  if(COMP(keyidentifier, "KEY_SYSTEM_SLEEP")) return KEY_SYSTEM_SLEEP;
  if(COMP(keyidentifier, "KEY_SYSTEM_WAKE_UP")) return KEY_SYSTEM_WAKE_UP;
  if(COMP(keyidentifier, "KEY_MINUS")) return KEY_MINUS;
  if(COMP(keyidentifier, "KEY_EQUAL")) return KEY_EQUAL;
  if(COMP(keyidentifier, "KEY_LEFT_BRACE")) return KEY_LEFT_BRACE;
  if(COMP(keyidentifier, "KEY_RIGHT_BRACE")) return KEY_RIGHT_BRACE;
  if(COMP(keyidentifier, "KEY_BACKSLASH")) return KEY_BACKSLASH;
  if(COMP(keyidentifier, "KEY_SEMICOLON")) return KEY_SEMICOLON;
  if(COMP(keyidentifier, "KEY_QUOTE")) return KEY_QUOTE;
  if(COMP(keyidentifier, "KEY_TILDE")) return KEY_TILDE;
  if(COMP(keyidentifier, "KEY_COMMA")) return KEY_COMMA;
  if(COMP(keyidentifier, "KEY_PERIOD")) return KEY_PERIOD;
  if(COMP(keyidentifier, "KEY_SLASH")) return KEY_SLASH;
  if(COMP(keyidentifier, "KEY_PRINTSCREEN")) return KEY_PRINTSCREEN;
  if(COMP(keyidentifier, "KEY_MENU")) return KEY_MENU;
  
  
  if(COMP(keyidentifier, "KEYPAD_SLASH")) return KEYPAD_SLASH;
  if(COMP(keyidentifier, "KEYPAD_ASTERIX")) return KEYPAD_ASTERIX;
  if(COMP(keyidentifier, "KEYPAD_MINUS")) return KEYPAD_MINUS;
  if(COMP(keyidentifier, "KEYPAD_PLUS")) return KEYPAD_PLUS;
  if(COMP(keyidentifier, "KEYPAD_ENTER")) return KEYPAD_ENTER;
  if(COMP(keyidentifier, "KEYPAD_1")) return KEYPAD_1;
  if(COMP(keyidentifier, "KEYPAD_2")) return KEYPAD_2;
  if(COMP(keyidentifier, "KEYPAD_3")) return KEYPAD_3;
  if(COMP(keyidentifier, "KEYPAD_4")) return KEYPAD_4;
  if(COMP(keyidentifier, "KEYPAD_5")) return KEYPAD_5;
  if(COMP(keyidentifier, "KEYPAD_6")) return KEYPAD_6;
  if(COMP(keyidentifier, "KEYPAD_7")) return KEYPAD_7;
  if(COMP(keyidentifier, "KEYPAD_8")) return KEYPAD_8;
  if(COMP(keyidentifier, "KEYPAD_9")) return KEYPAD_9;
  if(COMP(keyidentifier, "KEYPAD_0")) return KEYPAD_0;
  
  if(COMP(keyidentifier, "KEY_A")) return KEY_A;
  if(COMP(keyidentifier, "KEY_B")) return KEY_B;
  if(COMP(keyidentifier, "KEY_C")) return KEY_C;
  if(COMP(keyidentifier, "KEY_D")) return KEY_D;
  if(COMP(keyidentifier, "KEY_E")) return KEY_E;
  if(COMP(keyidentifier, "KEY_F")) return KEY_F;
  if(COMP(keyidentifier, "KEY_G")) return KEY_G;
  if(COMP(keyidentifier, "KEY_H")) return KEY_H;
  if(COMP(keyidentifier, "KEY_I")) return KEY_I;
  if(COMP(keyidentifier, "KEY_J")) return KEY_J;
  if(COMP(keyidentifier, "KEY_K")) return KEY_K;
  if(COMP(keyidentifier, "KEY_L")) return KEY_L;
  if(COMP(keyidentifier, "KEY_M")) return KEY_M;
  if(COMP(keyidentifier, "KEY_N")) return KEY_N;
  if(COMP(keyidentifier, "KEY_O")) return KEY_O;
  if(COMP(keyidentifier, "KEY_P")) return KEY_P;
  if(COMP(keyidentifier, "KEY_Q")) return KEY_Q;
  if(COMP(keyidentifier, "KEY_R")) return KEY_R;
  if(COMP(keyidentifier, "KEY_S")) return KEY_S;
  if(COMP(keyidentifier, "KEY_T")) return KEY_T;
  if(COMP(keyidentifier, "KEY_U")) return KEY_U;
  if(COMP(keyidentifier, "KEY_V")) return KEY_V;
  if(COMP(keyidentifier, "KEY_W")) return KEY_W;
  if(COMP(keyidentifier, "KEY_X")) return KEY_X;
  if(COMP(keyidentifier, "KEY_Y")) return KEY_Y;
  if(COMP(keyidentifier, "KEY_Z")) return KEY_Z;
  
  return 0;
}

/** @brief Parse a keycode to a key identifier
 * 
 * This method is used to parse a key code to a key identifier which
 * can be used for sending back the task_keyboard config.
 * 
 * @warning If you use key identifiers, no keyboard locale is taken into
 * account!
 * 
 * @param keycode Keycode to be parsed to a key identifier
 * @param buffer Char buffer where the key identifier is saved to
 * @param buf_len Length of buffer
 * @return 1 if found, 2 if buffer is too small, 0 if no key identifier was found
 * 
 * @see parseIdentifierToKeycode
 * */
uint16_t parseKeycodeToIdentifier(uint16_t keycode, char* buffer, uint8_t buf_len)
{
  switch(keycode)
  {
    case KEY_A: SAVE("KEY_A");
    case KEY_B: SAVE("KEY_B");
    case KEY_C: SAVE("KEY_C");
    case KEY_D: SAVE("KEY_D");
    case KEY_E: SAVE("KEY_E");
    case KEY_F: SAVE("KEY_F");
    case KEY_G: SAVE("KEY_G");
    case KEY_H: SAVE("KEY_H");
    case KEY_I: SAVE("KEY_I");
    case KEY_J: SAVE("KEY_J");
    case KEY_K: SAVE("KEY_K");
    case KEY_L: SAVE("KEY_L");
    case KEY_M: SAVE("KEY_M");
    case KEY_N: SAVE("KEY_N");
    case KEY_O: SAVE("KEY_O");
    case KEY_P: SAVE("KEY_P");
    case KEY_Q: SAVE("KEY_Q");
    case KEY_R: SAVE("KEY_R");
    case KEY_S: SAVE("KEY_S");
    case KEY_T: SAVE("KEY_T");
    case KEY_U: SAVE("KEY_U");
    case KEY_V: SAVE("KEY_V");
    case KEY_W: SAVE("KEY_W");
    case KEY_X: SAVE("KEY_X");
    case KEY_Y: SAVE("KEY_Y");
    case KEY_Z: SAVE("KEY_Z");
    
    case KEY_1: SAVE("KEY_1");
    case KEY_2: SAVE("KEY_2");
    case KEY_3: SAVE("KEY_3");
    case KEY_4: SAVE("KEY_4");
    case KEY_5: SAVE("KEY_5");
    case KEY_6: SAVE("KEY_6");
    case KEY_7: SAVE("KEY_7");
    case KEY_8: SAVE("KEY_8");
    case KEY_9: SAVE("KEY_9");
    case KEY_0: SAVE("KEY_0");
    
    case KEY_F1: SAVE("KEY_F1");
    case KEY_F2: SAVE("KEY_F2");
    case KEY_F3: SAVE("KEY_F3");
    case KEY_F4: SAVE("KEY_F4");
    case KEY_F5: SAVE("KEY_F5");
    case KEY_F6: SAVE("KEY_F6");
    case KEY_F7: SAVE("KEY_F7");
    case KEY_F8: SAVE("KEY_F8");
    case KEY_F9: SAVE("KEY_F9");
    case KEY_F10: SAVE("KEY_F10");
    case KEY_F11: SAVE("KEY_F11");
    case KEY_F12: SAVE("KEY_F12");
    case KEY_F13: SAVE("KEY_F13");
    case KEY_F14: SAVE("KEY_F14");
    case KEY_F15: SAVE("KEY_F15");
    case KEY_F16: SAVE("KEY_F16");
    case KEY_F17: SAVE("KEY_F17");
    case KEY_F18: SAVE("KEY_F18");
    case KEY_F19: SAVE("KEY_F19");
    case KEY_F20: SAVE("KEY_F20");
    case KEY_F21: SAVE("KEY_F21");
    case KEY_F22: SAVE("KEY_F22");
    case KEY_F23: SAVE("KEY_F23");
    case KEY_F24: SAVE("KEY_F24");
    
    case KEY_RIGHT: SAVE("KEY_RIGHT");
    case KEY_LEFT: SAVE("KEY_LEFT");
    case KEY_DOWN: SAVE("KEY_DOWN");
    case KEY_UP: SAVE("KEY_UP");
    
    case KEY_ENTER: SAVE("KEY_ENTER");
    case KEY_ESC: SAVE("KEY_ESC");
    case KEY_BACKSPACE: SAVE("KEY_BACKSPACE");
    case KEY_TAB: SAVE("KEY_TAB");
    case KEY_HOME: SAVE("KEY_HOME");
    case KEY_PAGE_UP: SAVE("KEY_PAGE_UP");
    case KEY_PAGE_DOWN: SAVE("KEY_PAGE_DOWN");
    case KEY_DELETE: SAVE("KEY_DELETE");
    case KEY_INSERT: SAVE("KEY_INSERT");
    case KEY_END: SAVE("KEY_END");
    
    case KEY_NUM_LOCK: SAVE("KEY_NUM_LOCK");
    case KEY_SCROLL_LOCK: SAVE("KEY_SCROLL_LOCK");
    case KEY_SPACE: SAVE("KEY_SPACE");
    case KEY_CAPS_LOCK: SAVE("KEY_CAPS_LOCK");
    case KEY_PAUSE: SAVE("KEY_PAUSE");
    case MODIFIERKEY_SHIFT: SAVE("KEY_SHIFT");
    case MODIFIERKEY_CTRL: SAVE("KEY_CTRL");
    case MODIFIERKEY_ALT: SAVE("KEY_ALT");
    case KEY_RIGHT_ALT: SAVE("KEY_RIGHT_ALT");
    case MODIFIERKEY_GUI: SAVE("KEY_GUI");
    case KEY_RIGHT_GUI: SAVE("KEY_RIGHT_GUI");
    
    case KEY_MEDIA_POWER: SAVE("KEY_MEDIA_POWER");
    case KEY_MEDIA_RESET: SAVE("KEY_MEDIA_RESET");
    case KEY_MEDIA_SLEEP: SAVE("KEY_MEDIA_SLEEP");
    case KEY_MEDIA_MENU: SAVE("KEY_MEDIA_MENU");
    case KEY_MEDIA_SELECTION: SAVE("KEY_MEDIA_SELECTION");
    case KEY_MEDIA_ASSIGN_SEL: SAVE("KEY_MEDIA_ASSIGN_SEL");
    case KEY_MEDIA_MODE_STEP: SAVE("KEY_MEDIA_MODE_STEP");
    case KEY_MEDIA_RECALL_LAST: SAVE("KEY_MEDIA_RECALL_LAST");
    case KEY_MEDIA_QUIT: SAVE("KEY_MEDIA_QUIT");
    case KEY_MEDIA_HELP: SAVE("KEY_MEDIA_HELP");
    case KEY_MEDIA_CHANNEL_UP: SAVE("KEY_MEDIA_CHANNEL_UP");
    case KEY_MEDIA_CHANNEL_DOWN: SAVE("KEY_MEDIA_CHANNEL_DOWN");
    case KEY_MEDIA_SELECT_DISC: SAVE("KEY_MEDIA_SELECT_DISC");
    case KEY_MEDIA_ENTER_DISC: SAVE("KEY_MEDIA_ENTER_DISC");
    case KEY_MEDIA_REPEAT: SAVE("KEY_MEDIA_REPEAT");
    case KEY_MEDIA_VOLUME: SAVE("KEY_MEDIA_VOLUME");
    case KEY_MEDIA_BALANCE: SAVE("KEY_MEDIA_BALANCE");
    case KEY_MEDIA_BASS: SAVE("KEY_MEDIA_BASS");
    
    case KEY_MEDIA_PLAY: SAVE("KEY_MEDIA_PLAY");
    case KEY_MEDIA_PAUSE: SAVE("KEY_MEDIA_PAUSE");
    case KEY_MEDIA_RECORD: SAVE("KEY_MEDIA_RECORD");
    case KEY_MEDIA_FAST_FORWARD: SAVE("KEY_MEDIA_FAST_FORWARD");
    case KEY_MEDIA_REWIND: SAVE("KEY_MEDIA_REWIND");
    case KEY_MEDIA_NEXT_TRACK: SAVE("KEY_MEDIA_NEXT_TRACK");
    case KEY_MEDIA_PREV_TRACK: SAVE("KEY_MEDIA_PREV_TRACK");
    case KEY_MEDIA_STOP: SAVE("KEY_MEDIA_STOP");
    case KEY_MEDIA_EJECT: SAVE("KEY_MEDIA_EJECT");
    case KEY_MEDIA_RANDOM_PLAY: SAVE("KEY_MEDIA_RANDOM_PLAY");
    case KEY_MEDIA_STOP_EJECT: SAVE("KEY_MEDIA_STOP_EJECT");
    case KEY_MEDIA_PLAY_PAUSE: SAVE("KEY_MEDIA_PLAY_PAUSE");
    case KEY_MEDIA_PLAY_SKIP: SAVE("KEY_MEDIA_PLAY_SKIP");
    case KEY_MEDIA_MUTE: SAVE("KEY_MEDIA_MUTE");
    case KEY_MEDIA_VOLUME_INC: SAVE("KEY_MEDIA_VOLUME_INC");
    case KEY_MEDIA_VOLUME_DEC: SAVE("KEY_MEDIA_VOLUME_DEC");
    
    case KEY_SYSTEM_POWER_DOWN: SAVE("KEY_SYSTEM_POWER_DOWN");
    case KEY_SYSTEM_SLEEP: SAVE("KEY_SYSTEM_SLEEP");
    case KEY_SYSTEM_WAKE_UP: SAVE("KEY_SYSTEM_WAKE_UP");
    case KEY_MINUS: SAVE("KEY_MINUS");
    case KEY_EQUAL: SAVE("KEY_EQUAL");
    case KEY_LEFT_BRACE: SAVE("KEY_LEFT_BRACE");
    case KEY_RIGHT_BRACE: SAVE("KEY_RIGHT_BRACE");
    case KEY_BACKSLASH: SAVE("KEY_BACKSLASH");
    case KEY_SEMICOLON: SAVE("KEY_SEMICOLON");
    case KEY_QUOTE: SAVE("KEY_QUOTE");
    case KEY_TILDE: SAVE("KEY_TILDE");
    case KEY_COMMA: SAVE("KEY_COMMA");
    case KEY_PERIOD: SAVE("KEY_PERIOD");
    case KEY_SLASH: SAVE("KEY_SLASH");
    case KEY_PRINTSCREEN: SAVE("KEY_PRINTSCREEN");
    case KEY_MENU: SAVE("KEY_MENU");
    
    case KEYPAD_SLASH: SAVE("KEYPAD_SLASH");
    case KEYPAD_ASTERIX: SAVE("KEYPAD_ASTERIX");
    case KEYPAD_MINUS: SAVE("KEYPAD_MINUS");
    case KEYPAD_PLUS: SAVE("KEYPAD_PLUS");
    case KEYPAD_ENTER: SAVE("KEYPAD_ENTER");
    case KEYPAD_1: SAVE("KEYPAD_1");
    case KEYPAD_2: SAVE("KEYPAD_2");
    case KEYPAD_3: SAVE("KEYPAD_3");
    case KEYPAD_4: SAVE("KEYPAD_4");
    case KEYPAD_5: SAVE("KEYPAD_5");
    case KEYPAD_6: SAVE("KEYPAD_6");
    case KEYPAD_7: SAVE("KEYPAD_7");
    case KEYPAD_8: SAVE("KEYPAD_8");
    case KEYPAD_9: SAVE("KEYPAD_9");
    case KEYPAD_0: SAVE("KEYPAD_0");
    //no keycode found
    default: return 0;
  }
  return 1;
}


/** @brief Get a keycode for the given UTF codepoint
 * 
 * This method parses a 16bit unicode character to a keycode.
 * If a modifier is needed the given modifier byte is updated.
 * 
 * @see keyboard_layouts
 * @param cpoint 16bit Unicode-codepoint to parse
 * @param locale Use this locale/layout to parse
 * @param keycode_modifier If a modifier is needed, it will be written here
 * @param deadkey_first_keystroke If a deadkey stroke is needed, it will be written here
 * @return 0 if no keycode is found; the keycode otherwise
 * */
uint8_t get_keycode(uint16_t cpoint,uint8_t locale,uint8_t *keycode_modifier, uint8_t *deadkey_first_keystroke)
{
  uint8_t part = cpoint & 0x00FF;
  uint8_t keycode = 0;
  keycode = parse_for_keycode(part,locale,keycode_modifier,deadkey_first_keystroke);
  //if the first part is evaluated as zero, try again with high byte
  if(keycode == 0)
  {
    part = (cpoint & 0xFF00) >> 8;
    keycode = parse_for_keycode(part,locale,keycode_modifier,deadkey_first_keystroke);
  }
  return keycode;
}

/** @brief Getting the HID country code for a given locale
 * 
 * Source: USB Device class definition for HID, page 33
 * 
 * @see keyboard_layouts
 * @return bCountryCode value for HID info
 * @param locale Locale number, as defined in keyboard_layouts
 **/
uint8_t get_hid_country_code(uint8_t locale)
{
  switch(locale)
  {
    case LAYOUT_US_ENGLISH:
    case LAYOUT_US_INTERNATIONAL:
      return 33;
      
    case LAYOUT_GERMAN:
    case LAYOUT_GERMAN_MAC:
      return 9;
    
    case LAYOUT_PORTUGUESE_BRAZILIAN:
    case LAYOUT_PORTUGUESE: 
      return 22;
      
    case LAYOUT_FRENCH_BELGIAN:
    case LAYOUT_FRENCH: 
      return 8;
    
    case LAYOUT_SPANISH_LATIN_AMERICA: /*same code for Latin America?*/
    case LAYOUT_SPANISH: 
      return 25;
    
    case LAYOUT_CANADIAN_FRENCH: return 04;
    case LAYOUT_CANADIAN_MULTILINGUAL: return 03;
    case LAYOUT_UNITED_KINGDOM: return 32;
    case LAYOUT_FINNISH: return 07;
    case LAYOUT_FRENCH_SWISS: return 27;
    case LAYOUT_DANISH: return 06;
    case LAYOUT_NORWEGIAN: return 19;
    case LAYOUT_SWEDISH: return 26;
    case LAYOUT_ITALIAN: return 14;
    case LAYOUT_GERMAN_SWISS: return 28;
    case LAYOUT_TURKISH: return 31;
    case LAYOUT_CZECH: return 5;
    //case LAYOUT_SERBIAN_LATIN_ONLY: 
    //  return 34; /*HID code for Yugoslavia?!?*/
    
    //now returning International, OK?
    //no defined country codes available
    case LAYOUT_IRISH:
    case LAYOUT_ICELANDIC:
      return 13;

    default:
      ESP_LOGE(LOG_TAG,"no hid country code found for locale %d",locale);
      return 0;
  }
}



