// I2c.cpp

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/i2c-dev.h> 

#include "I2c.h"

bool I2c::Open(uint8_t slave_address)
{
	if (!OpenDevice())
	{
		return false;
	}

	if (!SetSlaveAddress(slave_address))
	{
		Close();
		return false;
	}

	return true;
}

bool I2c::Close()
{
	if (m_fh > 0)
	{
		close(m_fh);
	}
	return true;
}

bool I2c::OpenDevice()
{
	// 'i2c_bus' is "/dev/i2c-1" (Gumstix, legacy...)
	//  Sadly we say Goodbye and Good Riddance to the stodgy, slow Gumstix platform.
	//      NEW: is "/dev/i2c-0" (NanoPi NEO PLUS platform  [2018]
	m_fh = open(i2c_bus, O_RDWR);

	if (m_fh < 0)
	{
		int myErr = errno;
		string s("Error: Can't open I2C bus: ");
		s += i2c_bus;
		s += ": ";
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}

	return true;
}

bool I2c::SetSlaveAddress(uint8_t address)
{
	if (m_fh < 0)
	{
		LogErr(AT, "Error: SetSlaveAddress, i2c bus is not open");
		return false;
	}
	// This WAS simply I2C_SLAVE
	// (_FORCE allows userspace access if another driver has ownership).
	if (ioctl(m_fh, I2C_SLAVE_FORCE, address) < 0)
	{
		int myErr = errno;
		string s("Error: Can't set slave address: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}

	return true;
}

bool I2c::WriteByte(uint8_t data)
{
	if (write(m_fh, &data, 1) != 1)
	{
		int myErr = errno;
		string s("Error requesting I2C status: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}
	return true;
}

bool I2c::ReadByte(uint8_t &data)
{
	uint8_t buf;
	if (read(m_fh, &buf, 1) != 1)
	{
		int myErr = errno;
		string s("Error reading I2C status: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}
	data = buf;
	return true;
}
