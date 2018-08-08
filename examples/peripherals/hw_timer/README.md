# _HW_TIMER Example_

* This example will show you how to use hw_timer by led or logic analyzer:  
    * Using hw_timer to generate waveforms of different frequencies
    * Observe the waveform with led or logic analyzer.    

## How to use example

### Hardware Required

 * Connect GPIO12 with led1
 * Connect GPIO15 with led2

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

## Example Output  

* LOG:  

```  
I (228) hw_timer_example: Config gpio
I (220) gpio: GPIO[12]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (224) gpio: GPIO[15]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (248) hw_timer_example: Initialize hw_timer for callback1
I (258) hw_timer_example: Set hw_timer timing time 100us with reload
I (1268) hw_timer_example: Deinitialize hw_timer for callback1
I (1261) hw_timer_example: Initialize hw_timer for callback2
I (1264) hw_timer_example: Set hw_timer timing time 1ms with reload
I (2278) hw_timer_example: Set hw_timer timing time 10ms with reload
I (4278) hw_timer_example: Set hw_timer timing time 100ms with reload
I (7278) hw_timer_example: Cancel timing
I (7270) hw_timer_example: Initialize hw_timer for callback3
I (7272) hw_timer_example: Set hw_timer timing time 1ms with one-shot

```  

* WAVE FORM:  

  ![wave](wave.png)  