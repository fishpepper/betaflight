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
#include "config/parameter_group.h"
#include <stdint.h>


typedef struct displayPortProfile_s {
    int8_t colAdjust;
    int8_t rowAdjust;

    uint8_t blackBrightness;
    uint8_t whiteBrightness;

    uint16_t supportedFeatures;
    uint16_t enabledFeatures;
} displayPortProfile_t;

typedef enum {
    DISPLAY_FEATURE_ENABLE           = (1 << 0),
    DISPLAY_FEATURE_INVERT           = (1 << 1),
    DISPLAY_FEATURE_BRIGHTNESS       = (1 << 2),
    DISPLAY_FEATURE_CHARSET          = (1 << 3),
    // 4..7
    DISPLAY_FEATURE_RENDER_LOGO      = (1 << 8),
    DISPLAY_FEATURE_RENDER_PILOTLOGO = (1 << 9),
    DISPLAY_FEATURE_RENDER_STICKS    = (1 << 10),
    DISPLAY_FEATURE_RENDER_SPECTRUM  = (1 << 11),
    DISPLAY_FEATURE_RENDER_CROSSHAIR = (1 << 12)
    // 13..15
} displayFeatures_e;


PG_DECLARE(displayPortProfile_t, displayPortProfile);

struct displayPortVTable_s;
typedef struct displayPort_s {
    const struct displayPortVTable_s *vTable;
    void *device;
    uint8_t rowCount;
    uint8_t colCount;
    uint8_t posX;
    uint8_t posY;

    // brightness
    uint8_t brightness_white;
    uint8_t brightness_black;

    // CMS state
    bool cleared;
    int8_t cursorRow;
    int8_t grabCount;
} displayPort_t;

typedef struct displayPortVTable_s {
    int (*grab)(displayPort_t *displayPort);
    int (*release)(displayPort_t *displayPort);
    int (*clearScreen)(displayPort_t *displayPort);
    int (*drawScreen)(displayPort_t *displayPort);
    int (*fillRegion)(displayPort_t *displayPort, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t value);
    int (*writeString)(displayPort_t *displayPort, uint8_t x, uint8_t y, const char *text);
    int (*writeChar)(displayPort_t *displayPort, uint8_t x, uint8_t y, uint8_t c);
    int (*reloadProfile)(displayPort_t *displayPort);
    bool (*isTransferInProgress)(const displayPort_t *displayPort);
    int (*heartbeat)(displayPort_t *displayPort);
    void (*resync)(displayPort_t *displayPort);
    uint32_t (*txBytesFree)(const displayPort_t *displayPort);
} displayPortVTable_t;

void displayGrab(displayPort_t *instance);
void displayRelease(displayPort_t *instance);
void displayReleaseAll(displayPort_t *instance);
bool displayIsGrabbed(const displayPort_t *instance);
void displayClearScreen(displayPort_t *instance);
void displayDrawScreen(displayPort_t *instance);
void displayFillRegion(displayPort_t *displayPort, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t value);
uint8_t displayScreenSizeRows(const displayPort_t *instance);
uint8_t displayScreenSizeCols(const displayPort_t *instance);
void displaySetXY(displayPort_t *instance, uint8_t x, uint8_t y);
int displayWrite(displayPort_t *instance, uint8_t x, uint8_t y, const char *s);
int displayWriteChar(displayPort_t *instance, uint8_t x, uint8_t y, uint8_t c);
int displayReloadProfile(displayPort_t *instance);
bool displayIsTransferInProgress(const displayPort_t *instance);
void displayHeartbeat(displayPort_t *instance);
void displayResync(displayPort_t *instance);
uint16_t displayTxBytesFree(const displayPort_t *instance);
void displayEnableFeature(displayPort_t *displayport, uint16_t features);
void displayDisableFeature(displayPort_t *displayport, uint16_t features);

void displayInit(displayPort_t *instance, const displayPortVTable_t *vTable);
