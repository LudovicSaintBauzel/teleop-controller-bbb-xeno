#!/bin/bash
echo cape-universaln > /sys/devices/bone_capemgr.9/slots
echo cape-bone-iio > /sys/devices/bone_capemgr.*/slots
sleep 0.001
echo 3 > /sys/class/pwm/export
sleep 0.001
echo 100000 > /sys/class/pwm/pwm3/period_ns
sleep 0.001
echo 1 > /sys/class/pwm/pwm3/run
sleep 0.001
echo 5 > /sys/class/pwm/export
sleep 0.001
echo 100000 > /sys/class/pwm/pwm5/period_ns
sleep 0.001
echo 1 > /sys/class/pwm/pwm5/run
sleep 0.001
#~ echo 0 > /sys/class/pwm/export
#~ sleep 0.001
#~ echo 1000 > /sys/class/pwm/pwm0/period_ns
#~ sleep 0.001
#~ echo 1 > /sys/class/pwm/pwm0/run
sleep 0.001
echo 65 > /sys/class/gpio/export
sleep 0.001
echo 60 > /sys/class/gpio/export
sleep 0.001
echo out >/sys/class/gpio/gpio65/direction
sleep 0.001
echo low > /sys/class/gpio/gpio65/direction
sleep 0.001
echo out >/sys/class/gpio/gpio60/direction
sleep 0.001
echo low > /sys/class/gpio/gpio60/direction
sleep 0.001
echo qep > /sys/devices/ocp.3/P9_92_pinmux.47/state
echo qep > /sys/devices/ocp.3/P9_27_pinmux.42/state
echo qep > /sys/devices/ocp.3/P8_11_pinmux.19/state
echo qep > /sys/devices/ocp.3/P8_12_pinmux.20/state
echo spi > /sys/devices/ocp.3/P9_17_pinmux.35/state
echo spi > /sys/devices/ocp.3/P9_18_pinmux.36/state
echo spi > /sys/devices/ocp.3/P9_21_pinmux.37/state
echo spi > /sys/devices/ocp.3/P9_22_pinmux.38/state
echo Init ended 
