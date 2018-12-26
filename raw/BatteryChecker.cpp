// BatteryChecker.cpp
// This used to be done in Diagnostics but we need a way to power down
// the unit when battery low, so BatterChecker thread is run by UntetheredOpManager
#include "BatteryChecker.h"

BatteryChecker::BatteryChecker(): ThreadRunnerBase("DiagnosticsThreadProc") { }

// For NanoPi NEO platform, we read:
// 1.
//  /sys/class/power_supply/battery/capacity to get 0-100% (integer)
//     -- gives capacity remaining in the battery
// 2.
//  /sys/class/power_supply/battery/voltage_now, similar to the original method.
//     -- gives the battery's voltage (16-bit number)
//    Take the number from #2, say 43536 / 12800 = battery voltage:
//                         43536
//                         -----  = 3.4V
//                         12800
// (Divide by 12800 is a simpler form of the same equation in 
//     Raw voltage:
// 1. and 2. are provided by a MAX17043 chip (which is I2C, but we have a
//   handy driver that reads these values for us).
// By reading simply the 'Capacity' we get 0-100.
// Zero means no remaining capacity.GetBatteryVoltageRaw
// I observed on brand new unit/new battery:
//    Capacity was 34 at 9:00 AM (approx), ran a survey and
//    flashed the LEDs for a few hours.
//    Around 10:15, gauge (capacity) was down to 7 but still going strong.
//    It got down to 5 at 10:23 <== here you have about 1/2 hour left.
//    Ran all the down to zero and for about 5 minutes at zero (bat volt: 3.265V)
//    and then the box FINALLY died at about 10:59...
// The MAX17043 also fires a processor interrupt when capacity gets too low
//   which the driver-level stuff will shut down all apps and the box
// This app no longer shuts anything down, but we will send BAT LOW warning at
// 5% capacity (about 1/2 hour left). Frank wants it to be higher, saying the
// battery might be damaged by running it all the way down. So we will see.
// For the first cut here, warn at 5% but never shut down.
// ========== VOLTAGE STATUS: defined in Proto message ================
// VoltageStatus is one of BATTERY_VOLTAGE_FAULT, LOw_VOLTAGE, NORMAL_VOLTAGE
//  (this is defined in the Proto file and is a big pain to alter,
//   Android app AND the orig 3.6 version all need updating if we do)
//

bool BatteryChecker::GetBatteryVoltageRaw(VoltageStatus& status, int& batteryCapacityPercent)
{
	string line;
	int battCapacity;  // 0% => 100% (yes it does run when at 0%, but not for long...)
	ifstream infile("/sys/class/power_supply/battery/capacity");
	if (!infile)
	{
		LogErr(AT, "Can't open battery capacity");
		status = VoltageStatus::BATTERY_VOLTAGE_FAULT;
		return false;
	}
	getline(infile, line);
	infile.close();
	
	istringstream iss(line);
	if (!(iss >> battCapacity))
	{
		string s("Error parsing batt capacity line: '");
		s += line;
		s += "'";
		LogErr(AT, s);
		status = VoltageStatus::BATTERY_VOLTAGE_FAULT;
		return false;
	}

	batteryCapacityPercent = battCapacity;
	// BatteryCapacityLow is now 10% (in BatteryChecker.h);
	// the device will shut down at 4%; this gives about half an hour
	// more operation before device turns itself off.
	status = 
		(batteryCapacityPercent > BatteryCapacityLow)
		?
		VoltageStatus::NORMAL_VOLTAGE
		:
		VoltageStatus::LOW_VOLTAGE;
	return true;
}

bool BatteryChecker::GetBatteryChargeStatus(ChargingStatus& status)
{
// Currently, no driver provides the Battery Charger IC's status,
//   so we read directly from I2C.
// "i2cdetect -r 0" - shows I2C device at 0x6b ("6b" means not taken by a driver)

// It would be great if the same driver that reads the battery status
// from the built-in MAX17043 chip (on the I2C bus at 0x6C) could
// also provide a FS entry for the Batt Charge Chip (at 0x6B)...    
	if (!m_i2c.start_transaction(0x6B))
	{
		// Failure reason already logged...
		status = ChargingStatus::NOT_CHARGING;
		return false;
	}

	// Request charging status:
	//   [This is where it fails if no device @0x6b, NOT at start_transaction().]
	if (!m_i2c.WriteByte(0x08))
	{
		// Failure reason already logged.
		m_i2c.end_transaction();
		status = ChargingStatus::NOT_CHARGING;
		return false;
	}

	// Get the charging status:
	uint8_t data;
	bool ok = m_i2c.ReadByte(data);
	m_i2c.end_transaction();
	if (!ok)
	{
		status = ChargingStatus::NOT_CHARGING;
		return false;
	}

	// ChargingStatus values are:
	// FAST_CHARGING, FULLY_CHARGED, NOT_CHARGING, PRE_CHARGING
	status = (ChargingStatus)(((data & 0x30) >> 4) + 1);

	return true;

}

// Diagnostics Thread Proc ALWAYS starts long after I start, as I start at power on.
// Diag gets the values I set from shared memory.
void BatteryChecker::_thread_proc(void)
{
	int batteryCapacityPercent;
	VoltageStatus status;
	ChargingStatus charging;
	bool rv;

	m_thread_is_running = true;
	SharedMemory sharedMemory;
	LogInfo("Starting BatteryCheckerThreadProc...");

	if (!sharedMemory.Open())
	{
		LogErr(AT, "Can't open shared memory.. aborting");
		m_thread_is_running = false;
		return;
	}
	SharedData *pShared = sharedMemory.m_pSharedData;

	while (m_thread_keep_running)
	{
		rv = GetBatteryChargeStatus(charging);
		pShared->batteryChargingStatus = charging;
		if (!rv)
		{
			// Can't read, exit this thread. Batt Charger chip not present?
			LogErr(AT, "BatteryChecker: Aborting thread proc(), charging status unavailable.");
			m_thread_keep_running = false;
			pShared->batteryVoltageStatus = VoltageStatus::BATTERY_VOLTAGE_FAULT;
			break;
		}

		// Currently, 'stat' goes to BATT_LOW when capacity <= 10%...
		// If can't get Battery Status, exit the thread
		//   My device for example doesn't have the chip built in yet.
		rv = GetBatteryVoltageRaw(status, batteryCapacityPercent);
		// We don't do anything with the Capacity Percent yet...
		// Will require updating the proto file which is a big
		// PIA; Android app has to rebuild and need to rebuild the
		// (legacy Gumstix) 3.6 version of this too.

		// Diagnostics runs when Android is attached; now reads
		// Voltage Status from shared memory.
		// This all USED TO BE in Diagnostics:
		pShared->batteryVoltageStatus = status;
		if (!rv)
		{
			// Can't read, exit this thread.
			LogErr(AT, "BatteryChecker: Aborting thread proc(), capacity unavailable.");
			m_thread_keep_running = false;
			break;
		}
        // NEW: I only send the Status up to Android appliance,
        //   it is up to OS to shutdown when battery low.
        // NOTE: It is OK if battery is LOW but also CHARGING.
		Sleep_ms(BatteryCheckInterval /* 15,000 milliseconds */);
	}  // while (m_thread_keep_running)
	
	LogInfo("Exiting BatteryCheckerThreadProc...");

	m_thread_is_running = false;

}
