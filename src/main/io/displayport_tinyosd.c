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

#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

#ifdef OSD

#include "common/utils.h"

#include "config/parameter_group.h"
#include "config/parameter_group_ids.h"

#include "drivers/vcd.h"
#include "drivers/display.h"
#include "drivers/tinyosd.h"

#include "io/displayport_tinyosd.h"
#include "io/osd.h"

displayPort_t tinyOSDDisplayPort;

// no template required since defaults are zero
PG_REGISTER(displayPortProfile_t, displayPortProfiletinyOSD, PG_DISPLAY_PORT_TINYOSD_CONFIG, 0);

static int grab(displayPort_t *displayPort)
{
    // FIXME this should probably not have a dependency on the OSD or OSD slave code
    UNUSED(displayPort);
#ifdef OSD
    osdResetAlarms();
    //resumeRefreshAt = 0;
#endif

    return 0;
}

static int release(displayPort_t *displayPort)
{
    UNUSED(displayPort);

    return 0;
}

static int clearScreen(displayPort_t *displayPort)
{
    UNUSED(displayPort);
    tinyOSDClearScreen();

    return 0;
}

static int drawScreen(displayPort_t *displayPort)
{
    UNUSED(displayPort);
    tinyOSDDrawScreen();

    return 0;
}

static int screenSize(const displayPort_t *displayPort)
{
    UNUSED(displayPort);
    return maxScreenSize;
}

static int write(displayPort_t *displayPort, uint8_t x, uint8_t y, const char *s)
{
    UNUSED(displayPort);
    tinyOSDWrite(x, y, s);

    return 0;
}

static int writeChar(displayPort_t *displayPort, uint8_t x, uint8_t y, uint8_t c)
{
    UNUSED(displayPort);
    tinyOSDWriteChar(x, y, c);

    return 0;
}

static bool isTransferInProgress(const displayPort_t *displayPort)
{
    UNUSED(displayPort);
    return 0;
}

static void resync(displayPort_t *displayPort)
{
    UNUSED(displayPort);
    tinyOSDRefreshAll();
    displayPort->rows = tinyOSDGetRowsCount() + displayPortProfiletinyOSD()->rowAdjust;
    displayPort->cols = 30 + displayPortProfiletinyOSD()->colAdjust;
}

static int heartbeat(displayPort_t *displayPort)
{
    UNUSED(displayPort);
    return 0;
}

static uint32_t txBytesFree(const displayPort_t *displayPort)
{
    UNUSED(displayPort);
    return UINT32_MAX;
}

static const displayPortVTable_t tinyOSDVTable = {
    .grab = grab,
    .release = release,
    .clearScreen = clearScreen,
    .drawScreen = drawScreen,
    .screenSize = screenSize,
    .write = write,
    .writeChar = writeChar,
    .isTransferInProgress = isTransferInProgress,
    .heartbeat = heartbeat,
    .resync = resync,
    .txBytesFree = txBytesFree,
};

displayPort_t *tinyOSDDisplayPortInit(const vcdProfile_t *vcdProfile)
{
    displayInit(&tinyOSDDisplayPort, &tinyOSDVTable);
    tinyOSDInit(vcdProfile);
    resync(&tinyOSDDisplayPort);
    return &tinyOSDDisplayPort;
}
#endif // USE_OSD
