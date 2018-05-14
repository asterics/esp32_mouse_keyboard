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
 
#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum keyboard_layouts{
  LAYOUT_US_ENGLISH,
  LAYOUT_US_INTERNATIONAL,
  LAYOUT_GERMAN,
  LAYOUT_GERMAN_MAC,
  LAYOUT_CANADIAN_FRENCH,
  LAYOUT_CANADIAN_MULTILINGUAL,
  LAYOUT_UNITED_KINGDOM,
  LAYOUT_FINNISH,
  LAYOUT_FRENCH,
  LAYOUT_DANISH,
  LAYOUT_NORWEGIAN,
  LAYOUT_SWEDISH,
  LAYOUT_SPANISH,
  LAYOUT_PORTUGUESE,
  LAYOUT_ITALIAN,
  LAYOUT_PORTUGUESE_BRAZILIAN,
  LAYOUT_FRENCH_BELGIAN,
  LAYOUT_GERMAN_SWISS,
  LAYOUT_FRENCH_SWISS,
  LAYOUT_SPANISH_LATIN_AMERICA,
  LAYOUT_IRISH,
  LAYOUT_ICELANDIC,
  LAYOUT_TURKISH,
  LAYOUT_CZECH,
  LAYOUT_SERBIAN_LATIN_ONLY,
  LAYOUT_MAX
};

/** parse an incoming byte for a keycode
 * 
 * This method parses one incoming byte for the given locale.
 * It returns 0 if there is no keycode or another byte is needed (Unicode input).
 * If a modifier is needed the given modifier byte is updated.
 * 
 * @see keyboard_layouts
 * @return 0 if another byte is needed or no keycode is found; the keycode otherwise
 * 
 * */
uint8_t parse_for_keycode(uint8_t inputdata, uint8_t locale, uint8_t *keycode_modifier, uint8_t *deadkey_first_keycode);

/** remove a keycode from the given HID keycode array.
 * 
 * @note The size of the keycode_arr parameter MUST be 6
 * @return 0 if the keycode was removed, 1 if the keycode was not in the array
 * */
uint8_t remove_keycode(uint8_t keycode,uint8_t *keycode_arr);

/** add a keycode to the given HID keycode array.
 * 
 * @note The size of the keycode_arr parameter MUST be 6
 * @return 0 if the keycode was added, 1 if the keycode was already in the array
 * */
uint8_t add_keycode(uint8_t keycode,uint8_t *keycode_arr);


/** get a keycode for the given UTF codepoint
 * 
 * This method parses a 16bit unicode character to a keycode.
 * If a modifier is needed the given modifier byte is updated.
 * 
 * @see keyboard_layouts
 * @return 0 if no keycode is found; the keycode otherwise
 * */
uint8_t get_keycode(uint16_t cpoint,uint8_t locale,uint8_t *keycode_modifier, uint8_t *deadkey_first_keystroke);


/** translate Unicode characters between different locales
 * 
 * This method translates a 16bit Unicode cpoint from one locale to another one.
 * 
 * @see keyboard_layouts
 * @return 0 if no cpoint is found; the cpoint otherwise
 * */
uint16_t get_cpoint(uint16_t cpoint,uint8_t locale_src,uint8_t locale_dst);

/** getting the HID country code for a given locale
 * 
 * @see keyboard_layouts
 * @return bCountryCode value for HID info
 * @param locale Locale number, as defined in keyboard_layouts
 **/
uint8_t get_hid_country_code(uint8_t locale);

#ifdef __cplusplus
}
#endif

#endif
 
