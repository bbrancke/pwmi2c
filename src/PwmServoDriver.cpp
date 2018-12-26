/*************************************************** 
  This is a library for our Adafruit 16-channel PWM & Servo driver

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/815

  These displays use I2C to communicate, 2 pins are required to  
  interface.

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/
// WAS: Adafruit_PWMServoDriver.cpp, NOW: PwmServoDriver.cpp

#include "PwmServoDriver.h"

// Set to true to print some debug messages, or false to disable them.
//#define ENABLE_DEBUG_OUTPUT


/**************************************************************************/
/*! 
    @brief  Instantiates a new PCA9685 PWM driver chip with the I2C address on the Wire interface. On Due we use Wire1 since its the interface on the 'default' I2C pins.
    @param  addr The 7-bit I2C address to locate this chip, default is 0x40
*/
/**************************************************************************/
PwmServoDriver::PwmServoDriver(uint8_t addr) {
	m_i2caddr = addr;
}

/**************************************************************************/
/*! 
    @brief  Setups the I2C interface and hardware
*/
/**************************************************************************/
void PwmServoDriver::begin(void) {
	// m_i2c.StartTransaction(m_i2caddr);
	reset();
	// set a default frequency
	setPWMFreq(4096);
}


/**************************************************************************/
/*! 
    @brief  Sends a reset command to the PCA9685 chip over I2C
*/
/**************************************************************************/
void PwmServoDriver::reset(void) {
  write8(PCA9685_MODE1, 0x80);
  delay(10);
}

/**************************************************************************/
/*! 
    @brief  Sets the PWM frequency for the entire chip, up to ~1.6 KHz
    @param  freq Floating point frequency that we will attempt to match
*/
/**************************************************************************/
void PwmServoDriver::setPWMFreq(float freq) {
#ifdef ENABLE_DEBUG_OUTPUT
  Serial.print("Attempting to set freq ");
  Serial.println(freq);
#endif
  // double floor(double x);
  // float floorf(float x);
  // Link with -lm.

	// Correct for overshoot in the frequency setting (see issue #11).
	freq *= 0.9;
	float prescaleval = 25000000;
	prescaleval /= 4096;
	prescaleval /= freq;
	prescaleval -= 1;

#ifdef ENABLE_DEBUG_OUTPUT
  Serial.print("Estimated pre-scale: "); Serial.println(prescaleval);
#endif

  uint8_t prescale = floorf(prescaleval + 0.5);
#ifdef ENABLE_DEBUG_OUTPUT
  Serial.print("Final pre-scale: "); Serial.println(prescale);
#endif
  
	uint8_t oldmode;
	read8(PCA9685_MODE1, oldmode);
	uint8_t newmode = (oldmode&0x7F) | 0x10; // sleep
	write8(PCA9685_MODE1, newmode); // go to sleep
	write8(PCA9685_PRESCALE, prescale); // set the prescaler
	write8(PCA9685_MODE1, oldmode);
	delay(5);
	write8(PCA9685_MODE1, oldmode | 0xa0);  //  This sets the MODE1 register to turn on auto increment.

#ifdef ENABLE_DEBUG_OUTPUT
  Serial.print("Mode now 0x"); Serial.println(read8(PCA9685_MODE1), HEX);
#endif
}

/**************************************************************************/
/*! 
    @brief  Sets the PWM output of one of the PCA9685 pins
    @param  num One of the PWM output pins, from 0 to 15
    @param  on At what point in the 4096-part cycle to turn the PWM output ON
    @param  off At what point in the 4096-part cycle to turn the PWM output OFF
*/
bool PwmServoDriver::setPWM(uint8_t num, uint16_t on, uint16_t off)
{
#ifdef ENABLE_DEBUG_OUTPUT
  Serial.print("Setting PWM "); Serial.print(num); Serial.print(": "); Serial.print(on); Serial.print("->"); Serial.println(off);
#endif

	return
		(
			m_i2c.Open(m_i2caddr)
			&&
			m_i2c.WriteByte(LED0_ON_L + (4 * num))
			&&
			m_i2c.WriteByte(LED0_ON_L + 4 * num)
			&&
			m_i2c.WriteByte(on & 0xff)
			&&
			m_i2c.WriteByte((on >> 8) & 0xff)
			&&
			m_i2c.WriteByte(off &0xff)
			&&
			m_i2c.WriteByte((off >> 8) & 0xff)
			&&
			m_i2c.Close()
		);
}

/**************************************************************************/
/*! 
    @brief  Helper to set pin PWM output. 
	           Sets pin without having to deal with on/off tick
			   placement and properly handles a zero value as
			   completely off and 4095 as completely on.
			   Optional invert parameter supports inverting
			   the pulse for sinking to ground.
    @param  num One of the PWM output pins, from 0 to 15
    @param  val The number of ticks out of 4096 to be active, should be a value from 0 to 4095 inclusive.
    @param  invert If true, inverts the output, defaults to 'false'
*/
void PwmServoDriver::setPin(uint8_t num, uint16_t val, bool invert)
{
  // Clamp value between 0 and 4095 inclusive.
	val = min(val, (uint16_t)4095);
	if (invert)
	{
		if (val == 0)
		{
			// Special value for signal fully on.
			setPWM(num, 4096, 0);
		}
		else if (val == 4095)
		{
			// Special value for signal fully off.
			setPWM(num, 0, 4096);
		}
		else
		{
			setPWM(num, 0, 4095-val);
		}
	}
	else
	{
		if (val == 4095)
		{
			// Special value for signal fully on.
			setPWM(num, 4096, 0);
		}
		else if (val == 0)
		{
			// Special value for signal fully off.
			setPWM(num, 0, 4096);
		}
		else
		{
			setPWM(num, 0, val);
		}
	}
}

/*******************************************************************************************/

bool PwmServoDriver::read8(uint8_t reg, uint8_t &val)
{
	return
		(
		m_i2c.Open(m_i2caddr)
		&&
		m_i2c.WriteByte(reg)
		&&
		m_i2c.ReadByte(val)
		&&
		m_i2c.Close()
		);

/*	
  _i2c->beginTransmission(_i2caddr);
  _i2c->write(addr);
  _i2c->endTransmission();

  _i2c->requestFrom((uint8_t)_i2caddr, (uint8_t)1);
  return _i2c->read();
*/
}

// example call:
// write8(PCA9685_PRESCALE, prescale); // set the prescaler
//   PCA9685_PRESCALE is 0xFE, prescale is 0-0xFF
bool PwmServoDriver::write8(uint8_t reg, uint8_t d) {
	return
		(
			m_i2c.Open(m_i2caddr)
			&&
			m_i2c.WriteByte(reg)
			&&
			m_i2c.WriteByte(d)
			&&
			m_i2c.Close()
		);

/*
  _i2c->beginTransmission(_i2caddr);
  _i2c->write(addr);
  _i2c->write(d);
  _i2c->endTransmission();
  */
}
/**********
 * Here is how we read Battery Charging Status:
 bool BatteryChecker::GetBatteryChargeStatus(ChargingStatus& status)
{
// "i2cdetect -r 0" - shows I2C device at 0x6b ("6b" means not taken by a driver)
	if (!m_i2c.start_transaction(0x6B)) { FAIL; ret false;}

	// Request charging status:
	//   [This is where it fails if no device @0x6b, NOT at start_transaction().]
	if (!m_i2c.WriteByte(0x08)) { close(); ret false;}

	// Get the charging status:
	uint8_t data;
	bool ok = m_i2c.ReadByte(data);
	m_i2c.end_transaction();
	if (!ok) { FAIL; ret false;}

	// ChargingStatus values are:
	// FAST_CHARGING, FULLY_CHARGED, NOT_CHARGING, PRE_CHARGING
	status = (ChargingStatus)(((data & 0x30) >> 4) + 1);

	return true;

}
 **********/