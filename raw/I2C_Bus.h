// I2C_Bus

#ifndef __I2C_BUS_H__
#define __I2C_BUS_H__

#include <iostream>
#include <string>
#include <sstream>

#include <stdint.h>

#include "../Log.h"

#define SLAVE_ADDRESS   0x54

class I2C_Bus : public Log
{
public:
	I2C_Bus() { SetLogName("I2C_Bus"); }
	bool start_transaction(uint8_t slave_address);
	bool end_transaction();
	bool WriteByte(uint8_t data);
	bool ReadByte(uint8_t &data);
private:
	// 2018: The new NanoPi NEO PLUS platform has ONE I2C bus, it is /dev/i2c-0:
	//       NOTE: This WAS i2c-2 in Shx 2, and ic2-1 in Gumstix original
	const char *i2c_bus = "/dev/i2c-0";
	int m_fh = -1;
	bool open_device();
	bool set_slave_address(uint8_t address);
};

#endif // __I2C_BUS_H__
