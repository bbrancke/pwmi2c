#! /bin/sh

echo "Building..."
cd ./src/

g++ -Wall Log.cpp I2c.cpp PwmServoDriver.cpp Main.cpp -lm -o pwm

cd ..
cp ./src/pwm ./

echo "Created 'pwm'"
