# _PWM Example_  

* This example will show you how to use PWM module by running four channels:  
    * Observe PWM signal with logic analyzer or oscilloscope.  

## Pin assignment  

    * GPIO12 is assigned as the PWM channel 0.  
    * GPIO13 is assigned as the PWM channel 1.  
    * GPIO14 is assigned as the PWM channel 2.  
    * GPIO15 is assigned as the PWM channel 3.  

## How to use example  

### Hardware Required  

* Connection:  
  * Connect the PWM channel to a logic analyzer or oscilloscope.  

### Configure the project  

```
make menuconfig
```

* Set serial port under Serial Flasher Options.


### Build and Flash  

Build the project and flash it to the board, then run monitor tool to view serial output:

```
make -j4 flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output  

* LOG:  

```  
I (220) gpio: GPIO[12]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (225) gpio: GPIO[13]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (247) gpio: GPIO[14]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (251) gpio: GPIO[15]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (265) pwm: --- PWM v3.2

I (20276) main: PWM stop

I (30276) main: PWM re-start

I (50276) main: PWM stop

I (60279) main: PWM re-start

I (80279) main: PWM stop

I (90279) main: PWM re-start

I (110272) main: PWM stop

I (120272) main: PWM re-start

```  

* WAVE FORM:  

  ![wave](wave.png)  