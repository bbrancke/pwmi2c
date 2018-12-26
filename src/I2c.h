// I2c.h

#ifndef I2C_H_
#define I2C_H_

#include <iostream>
#include <string>
#include <sstream>

#include <stdint.h>

#include "Log.h"

#define SLAVE_ADDRESS   0x54

class I2c : public Log
{
public:
	I2c() { SetLogName("I2c"); }
	bool Open(uint8_t slave_address);
	bool Close();
	bool WriteByte(uint8_t data);
	bool ReadByte(uint8_t &data);
private:
	// 2018: The new NanoPi NEO PLUS platform has ONE I2C bus, it is /dev/i2c-0:
	//       NOTE: This WAS i2c-2 in Shx 2, and ic2-1 in Gumstix original
	// /dev/i2c-1 on rpi3
	const char *i2c_bus = "/dev/i2c-1";
	int m_fh = -1;
	bool OpenDevice();
	bool SetSlaveAddress(uint8_t address);
};

#endif // __I2C_BUS_H__
