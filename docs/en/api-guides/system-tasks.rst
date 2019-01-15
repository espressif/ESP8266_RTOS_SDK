System Tasks
************

This document explains the ESP8266 RTOS SDK internal system tasks.

Overview
========

The main tasks and their attributes are as following:

+-------------------------+----------------+----------------+
| Names                   |   stack size   |    Priority    |
+=========================+================+================+
| uiT                     |     3584(C)    |       14       |
+-------------------------+----------------+----------------+
| IDLE                    |       768      |        0       |
+-------------------------+----------------+----------------+
| Tmr                     |     2048(C)    |        2       |
+-------------------------+----------------+----------------+
| ppT                     |     2048(C)    |       13       |
+-------------------------+----------------+----------------+
| pmT                     |      1024      |       11       |
+-------------------------+----------------+----------------+
| rtT                     |      2048      |       12       |
+-------------------------+----------------+----------------+
| tiT                     |     2048(C)    |        8       |
+-------------------------+----------------+----------------+
| esp_event_loop_task     |     2048(C)    |       10       |
+-------------------------+----------------+----------------+

Note: (C) means it is configurable by "menuconfig".

Tasks Introduction
==================

uiT
---

This task initializes the system, including peripherals, file system, user entry function and so on.
This task will delete itself and free the resources after calling `app_main`.

IDLE
----

This task is freeRTOS internal idle callback task, it is created when starting the freeRTOS.
Its hook function is `vApplicationIdleHook`.
The system's function of `sleep` and function of feeding `task watch dog` are called in the `vApplicationIdleHook`.

Tmr
---

This task is the processor of freeRTOS internal software timer.

ppT
---

This task is to process Wi-Fi hardware driver and stack. It posts messages from the logic link layer to the upper layer TCP/IP stack after transforming them into ethernet packets.

pmT
---

The task is for system power management. It will check if the system can sleep right now, and if it is, it will start preparing for system sleep.

rtT
---

The task is the processor of high priority hardware timer. It mainly process Wi-Fi real time events.
It is suggested that functions based on this component should not be called in application, because it may block other low layer Wi-Fi functions.

tiT
---

The task is the main task of TCP-IP stack(LwIP) , it is to deal with TCP-IP packets.

esp_event_loop_task
-------------------

The task processes system events, for example, Wi-Fi and TCP-IP stack events.

Suggestions
===========

In general, the priority of user task should NOT be higher than the system real timer task's priority (12). So it is suggested that keep your user tasks' priorities less than 12.  
If you want to speed up the TCP/UDP throughput, you can try to set the priority of send/receive task to be higher than the "tiT" task's priority (8).
