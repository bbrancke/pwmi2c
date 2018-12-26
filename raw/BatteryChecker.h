// BatteryChecker.h

#ifndef BATTERY_CHECKER_H_
#define BATTERY_CHECKER_H_

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <atomic>
#include <cstdio>

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/statvfs.h>

#include "I2C_Bus.h"
#include "../Log.h"
#include "../SharedMemory.h"
#include "../TextColor.h"
#include "../ThreadRunnerBase.h"
#include "../Shadowx_messages.pb.h"  // for VoltageStatus

using namespace std;

class BatteryChecker : public ThreadRunnerBase
{
public:
	BatteryChecker();
private:
	const int BatteryCheckInterval = 15000;  // milliseconds (15 secs)
	// This is how we USED to get battery voltage:
	// const char *AdcFile = "/sys/kernel/debug/twl6030_madc";
	
// ======= BATTERY CAPACITY THRESHOLD for "LOW BATTERY" INDICATOR =======	
	
	// For NanoPi NEO platform, we read battery percent capacity left.
	// Capacity (fully charged battery) appears to be about 84%,
	// when not charging, it slowly goes all the way zero.
	// Zero means battery has no remaining charge.
	// Driver conveniently provides the capacity value here:
	// "/sys/class/power_supply/battery/capacity"
	// ShadowX app no longer halts the CPU or anything drastic,
	// that is now up to the OS which gets an ALERT interupt
	// on GPIO11 when battery is very low.
	// ShadowX WILL send BATTERY_LOW to Android when capacity
	// is <= this:
	// Device will shutdown when Battery gets to 4% (there's an interrupt for this)
	// so alert at 10% capacity (about 1/2 hour remaining time).
	const int BatteryCapacityLow = 10;
    // (At 5%, you have about one-half hour left!)
	I2C_Bus m_i2c;
	bool GetBatteryVoltageRaw(VoltageStatus& status, int& batteryCapacityPercent);
	bool GetBatteryChargeStatus(ChargingStatus& status);
	void _thread_proc(void);
};

#endif // BATTERY_CHECKER_H_
