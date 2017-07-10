#!/bin/bash
echo 0 > /sys/class/gpio/gpio60/value
echo 0 > /sys/class/gpio/gpio65/value
#~ rmmod motor1
#~ rmmod motor2
rmmod force_sensor1
rmmod force_sensor2
rmmod pwm 
 
