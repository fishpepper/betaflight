/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#ifndef WHITEBRIGHTNESS
  #define WHITEBRIGHTNESS 0x01
#endif
#ifndef BLACKBRIGHTNESS
  #define BLACKBRIGHTNESS 0x00
#endif

/** PAL or NTSC, value is number of chars total */
#define TINYOSD_VIDEO_LINES_NTSC          13
#define TINYOSD_VIDEO_LINES_PAL           16
#define TINYOSD_VIDEO_BUFFER_CHARS_NTSC   (35 * TINYOSD_VIDEO_LINES_NTSC)
#define TINYOSD_VIDEO_BUFFER_CHARS_PAL    (35 * TINYOSD_VIDEO_LINES_PAL)

#define SCREEN_BUFFER_CHANGED(__pos) (screenBuffer[(__pos)] != shadowBuffer[(__pos)])


extern uint16_t maxScreenSize;
#define TINYOSD_CRC8_UPDATE(__crc, __val) ((__crc) = tinyosd_crc8_table[(__crc) ^ (__val)])
#define TINYOSD_CRC8_INIT(__crc, __ival) { (__crc) = (__ival); }


struct vcdProfile_s;
void    tinyOSDHardwareReset(void);
void    tinyOSDInit(const struct vcdProfile_s *vcdProfile);
void    tinyOSDDrawScreen(void);
void    tinyOSDWriteNvm(uint8_t char_address, const uint8_t *font_data);
uint8_t tinyOSDGetRowsCount(void);
void    tinyOSDWrite(uint8_t x, uint8_t y, const char *buff);
void    tinyOSDWriteChar(uint8_t x, uint8_t y, uint8_t c);
void    tinyOSDClearScreen(void);
void    tinyOSDRefreshAll(void);
uint8_t* tinyOSDGetScreenBuffer(void);
