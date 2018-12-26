#include <ostream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>

using namespace std;
using namespace chrono;

#include "Log.h"
#include "PwmServoDriver.h"

int main(int argc, char *argv[])
{
    PwmServoDriver pwm(0x40);
    // Reset chip, set freq to 4096:
    pwm.begin();

    // bool PwmServoDriver::setPWM(uint8_t num, uint16_t on, uint16_t off)
    //     num: One of the PWM output pins, from 0 to 15
    //      on: At what point in the 4096 - part cycle
    //          to turn the PWM output ON
    //     off: At what point in the 4096 - part cycle to turn the
    //          PWM output OFF
    // Note: Power supply is 12V, motor is rated a 6V so don't turn it on
    //       more than 1/2 the time!
    // 1-500: ~1/8 power:
    pwm.setPWM(0, 1, 500);

    this_thread::sleep_for(milliseconds(4000));

    // Turn it fully OFF:
    pwm.setPWM(0, 0, 4096);

    // Still need to use the DIRECTION GPIO for FWD / REV...


}