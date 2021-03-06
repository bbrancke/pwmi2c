/*************************************************** 
  This is a library for our Adafruit 16-channel PWM & Servo driver

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/815

  These displays use I2C to communicate, 2 pins are required to  
  interface. For Arduino UNOs, thats SCL -> Analog 5, SDA -> Analog 4

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/
// WAS Adafruit_PWMServoDriver.h, NOW: PwmServoDriver.h
#ifndef PWMSERVODRIVER_H_
#define PWMSERVODRIVER_H_

#include <thread>
#include <chrono>

#include <math.h>

#include "I2c.h"

using namespace std;
using namespace chrono;

#define PCA9685_SUBADR1 0x2
#define PCA9685_SUBADR2 0x3
#define PCA9685_SUBADR3 0x4

#define PCA9685_MODE1 0x0
#define PCA9685_PRESCALE 0xFE

#define LED0_ON_L 0x6
#define LED0_ON_H 0x7
#define LED0_OFF_L 0x8
#define LED0_OFF_H 0x9

#define ALLLED_ON_L 0xFA
#define ALLLED_ON_H 0xFB
#define ALLLED_OFF_L 0xFC
#define ALLLED_OFF_H 0xFD

//  Interact with PCA9685 PWM chip
class PwmServoDriver {
public:
	PwmServoDriver(uint8_t addr = 0x40);
	void begin(void);
	void reset(void);
	void setPWMFreq(float freq);
	bool setPWM(uint8_t num, uint16_t on, uint16_t off);
	void setPin(uint8_t num, uint16_t val, bool invert=false);

private:
	uint8_t m_i2caddr;
	I2c m_i2c;
	bool read8(uint8_t reg, uint8_t &val);
	bool write8(uint8_t reg, uint8_t d);
	void delay(int n)
	{
		this_thread::sleep_for(milliseconds(n));       
	}
};

#endif  // PWMSERVO_DRIVER_H_
