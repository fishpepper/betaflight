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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern "C" {
    #include "platform.h"

    #include "build/debug.h"

    #include "blackbox/blackbox.h"

    #include "config/parameter_group_ids.h"

    #include "drivers/max7456_symbols.h"

    #include "fc/config.h"
    #include "fc/rc_controls.h"
    #include "fc/rc_modes.h"
    #include "fc/runtime_config.h"

    #include "flight/pid.h"
    #include "flight/imu.h"

    #include "io/gps.h"
    #include "io/osd.h"

    #include "sensors/battery.h"

    #include "rx/rx.h"

    void osdRefresh(timeUs_t currentTimeUs);

    uint8_t stateFlags;
    uint8_t armingFlags;
    uint16_t flightModeFlags;
    uint16_t rssi;
    attitudeEulerAngles_t attitude;
    pidProfile_t *currentPidProfile;
    int16_t debug[DEBUG16_VALUE_COUNT];
    int16_t rcData[MAX_SUPPORTED_RC_CHANNEL_COUNT];
    uint8_t GPS_numSat;
    uint16_t GPS_distanceToHome;
    uint16_t GPS_directionToHome;
    int32_t GPS_coord[2];
    gpsSolutionData_t gpsSol;

    PG_REGISTER(batteryConfig_t, batteryConfig, PG_BATTERY_CONFIG, 0);
    PG_REGISTER(blackboxConfig_t, blackboxConfig, PG_BLACKBOX_CONFIG, 0);
    PG_REGISTER(systemConfig_t, systemConfig, PG_SYSTEM_CONFIG, 0);

    timeUs_t simulationTime = 0;
    batteryState_e simulationBatteryState;
    uint8_t simulationBatteryCellCount;
    uint16_t simulationBatteryVoltage;
    int32_t simulationAltitude;
    int32_t simulationVerticalSpeed;
}

/* #define DEBUG_OSD */

#include "unittest_macros.h"
#include "unittest_displayport.h"
#include "gtest/gtest.h"

void setDefualtSimulationState()
{
    rssi = 1024;

    simulationBatteryState = BATTERY_OK;
    simulationBatteryCellCount = 4;
    simulationBatteryVoltage = 168;
    simulationAltitude = 0;
    simulationVerticalSpeed = 0;
}

/*
 * Performs a test of the OSD actions on arming.
 * (reused throughout the test suite)
 */
void doTestArm(bool testEmpty = true)
{
    // given
    // craft has been armed
    ENABLE_ARMING_FLAG(ARMED);

    // when
    // sufficient OSD updates have been called
    osdRefresh(simulationTime);

    // then
    // arming alert displayed
    displayPortTestBufferSubstring(12, 7, "ARMED");

    // given
    // armed alert times out (0.5 seconds)
    simulationTime += 0.5e6;

    // when
    // sufficient OSD updates have been called
    osdRefresh(simulationTime);

    // then
    // arming alert disappears
#ifdef DEBUG_OSD
    displayPortTestPrint();
#endif
    if (testEmpty) {
        displayPortTestBufferIsEmpty();
    }
}

/*
 * Performs a test of the OSD actions on disarming.
 * (reused throughout the test suite)
 */
void doTestDisarm()
{
    // given
    // craft is disarmed after having been armed
    DISABLE_ARMING_FLAG(ARMED);

    // when
    // sufficient OSD updates have been called
    osdRefresh(simulationTime);

    // then
    // post flight statistics displayed
    displayPortTestBufferSubstring(2, 2, "  --- STATS ---");
}


/*
 * Tests initialisation of the OSD and the power on splash screen.
 */
TEST(OsdTest, TestInit)
{
    // given
    // display port is initialised
    displayPortTestInit();

    // and
    // default state values are set
    setDefualtSimulationState();

    // and
    // this battery configuration (used for battery voltage elements)
    batteryConfigMutable()->vbatmincellvoltage = 33;
    batteryConfigMutable()->vbatmaxcellvoltage = 43;

    // when
    // OSD is initialised
    osdInit(&testDisplayPort);

    // then
    // display buffer should contain splash screen
    displayPortTestBufferSubstring(7, 8, "MENU: THR MID");
    displayPortTestBufferSubstring(11, 9, "+ YAW LEFT");
    displayPortTestBufferSubstring(11, 10, "+ PITCH UP");

    // when
    // splash screen timeout has elapsed
    simulationTime += 4e6;
    osdUpdate(simulationTime);

    // then
    // display buffer should be empty
#ifdef DEBUG_OSD
    displayPortTestPrint();
#endif
    displayPortTestBufferIsEmpty();
}

/*
 * Tests visibility of the ARMED notification after arming.
 */
TEST(OsdTest, TestArm)
{
    doTestArm();
}

/*
 * Tests display and timeout of the post flight statistics screen after disarming.
 */
TEST(OsdTest, TestDisarm)
{
    doTestDisarm();

    // given
    // post flight stats times out (60 seconds)
    simulationTime += 60e6;

    // when
    // sufficient OSD updates have been called
    osdRefresh(simulationTime);

    // then
    // post flight stats screen disappears
#ifdef DEBUG_OSD
    displayPortTestPrint();
#endif
    displayPortTestBufferIsEmpty();
}

/*
 * Tests disarming and immediately rearming clears post flight stats and shows ARMED notification.
 */
TEST(OsdTest, TestDisarmWithImmediateRearm)
{
    doTestArm();
    doTestDisarm();
    doTestArm();
}

/*
 * Tests dismissing the statistics screen with pitch stick after disarming.
 */
TEST(OsdTest, TestDisarmWithDismissStats)
{
    // Craft is alread armed after previous test

    doTestDisarm();

    // given
    // sticks have been moved
    rcData[PITCH] = 1800;

    // when
    // sufficient OSD updates have been called
    osdRefresh(simulationTime);
    osdRefresh(simulationTime);

    // then
    // post flight stats screen disappears
#ifdef DEBUG_OSD
    displayPortTestPrint();
#endif
    displayPortTestBufferIsEmpty();

    rcData[PITCH] = 1500;
}

/*
 * Tests the calculation of statistics with imperial unit output.
 */
TEST(OsdTest, TestStatsImperial)
{
    // given
    // this set of enabled post flight statistics
    osdConfigMutable()->enabled_stats[OSD_STAT_MAX_SPEED]       = true;
    osdConfigMutable()->enabled_stats[OSD_STAT_MIN_BATTERY]     = true;
    osdConfigMutable()->enabled_stats[OSD_STAT_MIN_RSSI]        = true;
    osdConfigMutable()->enabled_stats[OSD_STAT_MAX_CURRENT]     = false;
    osdConfigMutable()->enabled_stats[OSD_STAT_USED_MAH]        = false;
    osdConfigMutable()->enabled_stats[OSD_STAT_MAX_ALTITUDE]    = true;
    osdConfigMutable()->enabled_stats[OSD_STAT_BLACKBOX]        = false;
    osdConfigMutable()->enabled_stats[OSD_STAT_END_BATTERY]     = true;
    osdConfigMutable()->enabled_stats[OSD_STAT_FLYTIME]         = true;
    osdConfigMutable()->enabled_stats[OSD_STAT_ARMEDTIME]       = true;
    osdConfigMutable()->enabled_stats[OSD_STAT_MAX_DISTANCE]    = true;
    osdConfigMutable()->enabled_stats[OSD_STAT_BLACKBOX_NUMBER] = false;

    // and
    // using imperial unit system
    osdConfigMutable()->units = OSD_UNIT_IMPERIAL;

    // and
    // a GPS fix is present
    stateFlags |= GPS_FIX | GPS_FIX_HOME;

    // when
    // the craft is armed
    doTestArm();

    // and
    // these conditions occur during flight
    rssi = 1024;
    gpsSol.groundSpeed = 500;
    GPS_distanceToHome = 20;
    simulationBatteryVoltage = 158;
    simulationAltitude = 100;
    simulationTime += 1e6;
    osdRefresh(simulationTime);

    rssi = 512;
    gpsSol.groundSpeed = 800;
    GPS_distanceToHome = 50;
    simulationBatteryVoltage = 147;
    simulationAltitude = 150;
    simulationTime += 1e6;
    osdRefresh(simulationTime);

    rssi = 256;
    gpsSol.groundSpeed = 200;
    GPS_distanceToHome = 100;
    simulationBatteryVoltage = 152;
    simulationAltitude = 200;
    simulationTime += 1e6;
    osdRefresh(simulationTime);

    // and
    // the craft is disarmed
    doTestDisarm();

    // then
    // statistics screen should display the following
    displayPortTestBufferSubstring(2, 3,  "ARMED TIME        : 00:04");
    displayPortTestBufferSubstring(2, 4,  "FLY TIME          : 00:07");
    displayPortTestBufferSubstring(2, 5,  "MAX SPEED         : 28");
    displayPortTestBufferSubstring(2, 6,  "MAX DISTANCE      : 328%c", SYM_FT);
    displayPortTestBufferSubstring(2, 7,  "MIN BATTERY       : 14.7%c", SYM_VOLT);
    displayPortTestBufferSubstring(2, 8,  "END BATTERY       : 15.2%c", SYM_VOLT);
    displayPortTestBufferSubstring(2, 9,  "MIN RSSI          : 25%%");
    displayPortTestBufferSubstring(2, 10, "MAX ALTITUDE      :  6.5%c", SYM_FT);
}

/*
 * Tests the calculation of statistics with metric unit output.
 * (essentially an abridged version of the previous test
 */
TEST(OsdTest, TestStatsMetric)
{
    // given
    // using metric unit system
    osdConfigMutable()->units = OSD_UNIT_METRIC;

    // and
    // default state values are set
    setDefualtSimulationState();

    // when
    // the craft is armed
    doTestArm();

    // and
    // these conditions occur during flight (simplified to less assignments than previous test)
    rssi = 256;
    gpsSol.groundSpeed = 800;
    GPS_distanceToHome = 100;
    simulationBatteryVoltage = 147;
    simulationAltitude = 200;
    simulationTime += 1e6;
    osdRefresh(simulationTime);
    osdRefresh(simulationTime);

    simulationBatteryVoltage = 152;
    simulationTime += 1e6;
    osdRefresh(simulationTime);

    // and
    // the craft is disarmed
    doTestDisarm();

    // then
    // statistics screen should display the following
    displayPortTestBufferSubstring(2, 3,  "ARMED TIME        : 00:02");
    displayPortTestBufferSubstring(2, 4,  "FLY TIME          : 00:09");
    displayPortTestBufferSubstring(2, 5,  "MAX SPEED         : 28");
    displayPortTestBufferSubstring(2, 6,  "MAX DISTANCE      : 100%c", SYM_M);
    displayPortTestBufferSubstring(2, 7,  "MIN BATTERY       : 14.7%c", SYM_VOLT);
    displayPortTestBufferSubstring(2, 8,  "END BATTERY       : 15.2%c", SYM_VOLT);
    displayPortTestBufferSubstring(2, 9,  "MIN RSSI          : 25%%");
    displayPortTestBufferSubstring(2, 10, "MAX ALTITUDE      :  2.0%c", SYM_M);
}

/*
 * Tests activation of alarms and element flashing.
 */
TEST(OsdTest, TestAlarms)
{
    // given
    // default state is set
    setDefualtSimulationState();

    // and
    // the following OSD elements are visible
    osdConfigMutable()->item_pos[OSD_RSSI_VALUE] = (osdItemPos_t) {   8,  1, OSD_ORIGIN_NW};
    osdConfigMutable()->visible[OSD_RSSI_VALUE] = true;

    osdConfigMutable()->item_pos[OSD_MAIN_BATT_VOLTAGE] = (osdItemPos_t) {   12,  1, OSD_ORIGIN_NW};
    osdConfigMutable()->visible[OSD_MAIN_BATT_VOLTAGE] = true;

    osdConfigMutable()->item_pos[OSD_FLYTIME] = (osdItemPos_t) {   1,  1, OSD_ORIGIN_NW};
    osdConfigMutable()->visible[OSD_FLYTIME] = true;

    osdConfigMutable()->item_pos[OSD_ALTITUDE] = (osdItemPos_t) {   23,  7, OSD_ORIGIN_NW};
    osdConfigMutable()->visible[OSD_ALTITUDE] = true;

    // and
    // this set of alarm values
    osdConfigMutable()->rssi_alarm = 20;
    osdConfigMutable()->cap_alarm  = 2200;
    osdConfigMutable()->time_alarm = 1; // in minutes
    osdConfigMutable()->alt_alarm  = 100; // meters

    // and
    // using the metric unit system
    osdConfigMutable()->units = OSD_UNIT_METRIC;

    // when
    // the craft is armed
    doTestArm(false);

    // then
    // no elements should flash as all values are out of alarm range
    for (int i = 0; i < 30; i++) {
        // Check for visibility every 100ms, elements should always be visible
        simulationTime += 0.1e6;
        osdRefresh(simulationTime);

#ifdef DEBUG_OSD
        printf("%d\n", i);
#endif
        displayPortTestBufferSubstring(8,  1, "%c99", SYM_RSSI);
        displayPortTestBufferSubstring(12, 1, "%c16.8%c", SYM_BATT_FULL, SYM_VOLT);
        displayPortTestBufferSubstring(1,  1, "%c00:", SYM_FLY_M); // only test the minute part of the timer
        displayPortTestBufferSubstring(23, 7, " 0.0%c", SYM_M);
    }

    // when
    // all values are out of range
    rssi = 128;
    simulationBatteryState = BATTERY_CRITICAL;
    simulationBatteryVoltage = 135;
    simulationAltitude = 12000;
    // Fly timer is incremented on periodic calls to osdRefresh, can't simply just increment the simulated system clock
    for (int i = 0; i < 60; i++) {
        simulationTime += 1e6;
        osdRefresh(simulationTime);
    }

    // then
    // elements showing values in alarm range should flash
    for (int i = 0; i < 15; i++) {
        // Blinking should happen at 5Hz
        simulationTime += 0.2e6;
        osdRefresh(simulationTime);

#ifdef DEBUG_OSD
        printf("%d\n", i);
        displayPortTestPrint();
#endif
        if (i % 2 == 0) {
            displayPortTestBufferSubstring(8,  1, "%c12", SYM_RSSI);
            displayPortTestBufferSubstring(12, 1, "%c13.5%c", SYM_MAIN_BATT, SYM_VOLT);
            displayPortTestBufferSubstring(1,  1, "%c01:", SYM_FLY_M); // only test the minute part of the timer
            displayPortTestBufferSubstring(23, 7, " 120.0%c", SYM_M);
        } else {
            displayPortTestBufferIsEmpty();
        }
    }
}

/*
 * Tests the RSSI OSD element.
 */
TEST(OsdTest, TestElementRssi)
{
    // given
    osdConfigMutable()->item_pos[OSD_RSSI_VALUE] = (osdItemPos_t) {   8,  1, OSD_ORIGIN_NW};
    osdConfigMutable()->visible[OSD_RSSI_VALUE] = true;
    osdConfigMutable()->rssi_alarm = 0;

    // when
    rssi = 1024;
    displayClearScreen(&testDisplayPort);
    osdRefresh(simulationTime);

    // then
    displayPortTestBufferSubstring(8, 1, "%c99", SYM_RSSI);

    // when
    rssi = 0;
    displayClearScreen(&testDisplayPort);
    osdRefresh(simulationTime);

    // then
    displayPortTestBufferSubstring(8, 1, "%c0", SYM_RSSI);

    // when
    rssi = 512;
    displayClearScreen(&testDisplayPort);
    osdRefresh(simulationTime);

    // then
    displayPortTestBufferSubstring(8, 1, "%c50", SYM_RSSI);
}


// STUBS
extern "C" {
    bool sensors(uint32_t mask) {
        UNUSED(mask);
        return true;
    }

    bool IS_RC_MODE_ACTIVE(boxId_e boxId) {
        UNUSED(boxId);
        return false;
    }

    uint32_t micros() {
        return simulationTime;
    }

    bool isBeeperOn() {
        return false;
    }

    bool isAirmodeActive() {
        return false;
    }

    uint8_t getCurrentPidProfileIndex() {
        return 0;
    }

    uint8_t getCurrentControlRateProfileIndex() {
        return 0;
    }

    batteryState_e getBatteryState() {
        return simulationBatteryState;
    }

    uint8_t getBatteryCellCount() {
        return simulationBatteryCellCount;
    }

    uint16_t getBatteryVoltage() {
        return simulationBatteryVoltage;
    }

    int32_t getAmperage() {
        return 0;
    }

    int32_t getMAhDrawn() {
        return 0;
    }

    int32_t getEstimatedAltitude() {
        return simulationAltitude;
    }

    int32_t getEstimatedVario() {
        return simulationVerticalSpeed;
    }

    unsigned int blackboxGetLogNumber() {
        return 0;
    }

    bool isSerialTransmitBufferEmpty(const serialPort_t *instance) {
        UNUSED(instance);
        return false;
    }

    void serialWrite(serialPort_t *instance, uint8_t ch) {
        UNUSED(instance);
        UNUSED(ch);
    }

    bool cmsDisplayPortRegister(displayPort_t *pDisplay) {
        UNUSED(pDisplay);
        return false;
    }

    void setArmingDisabled(armingDisableFlags_e flag) { UNUSED(flag); }
    void unsetArmingDisabled(armingDisableFlags_e flag) { UNUSED(flag); }
}
