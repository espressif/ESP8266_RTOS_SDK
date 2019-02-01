PWM & Sniffer Co-exists
***********************

1. Overview
===========

Without hardware PWM, ESP8266 has to use the hardware timer to simulate the PWM. We are using the Wi-Fi internal timer to drive the PWM, so there may be resource competition issue when using PWM and sniffer/SmartConfig at the same time. 


2. Root Cause
=============

To ensure the high precision of the PWM, the hardware Timer1 will trigger the interrupt AHEAD_TICKS1(6us by default) earlier. And in the interrupt, it will poll to wait for AHEAD_TICKS1(6us by default).
After handling the GPIO invert in one channel, the system will check the remaining time (T1) to the next channel invert.

If the T1 < AHEAD_TICKS2(8us by default), the system will not exit the interrupt, but poll to wait till timeout, and then invert the GPIO in the next channel; then the system will repeat these steps until all channels inverted.

So theoretically, the max time that PWM may occupy the CPU is 6 + 8 * n, n means the channel count. For example, if there are 3 channels, then PWM may take 30us at most.

In this case, PWM will affect the Wi-Fi sniffer/SmartConfig function, especially for the capture of the LDPC packets, or HT40 packets which require the CPU to handle them in time, otherwise those packets will loss. 


3. Issue that may happen
========================

If your application used both PWM and sniffer/SmartConfig, the sniffer/SmartConfig may take a long time to connect to an AP.   
You can stop the PWM and try it again. If the sniffer/SmartConfig becomes much faster, then it is the PWM that affect the sniffer/SmartConfig. In this case, you should adjust the frequency, duty cycle and phase of the PWM.

4. Suggestion
=============

When using the PWM and SmartConfig at the same time, please note:

1. The PWM's frequency cannot be too high, 2KHz at most.
2. Revise the PWM's duty cycle and phase, make the time intervals (Tn) between each channel inverting be equal to 0 or be larger than 50us (Tn = 0, or Tn > 50).
