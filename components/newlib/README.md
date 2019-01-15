The newlib version has been updated to "2.2.0" and supply two version C libraries to user.
One is "libc.a" which functions have more features base on stand C, another is "libc_nano.c"
which functions have less function.

You can choose the one you needed by menuconfig:
```
Component config --->
        newlib --- >
                newlib level(XXXX)
                        ( ) normal
                        ( ) nano
```

The normal "libc" has position argument, "long long" type data and float data
transformation at function printf/scanf and so on. So it should cost more stack
and heap, the test data as following:

|||||||||||||||
|--:|--:|--:|--:|--:|--:|--:|--:|--:|--:|--:|--:|--:|--:|
||Object||Origin newlib||||New newlib||||New newlib nano|||
|||||||||||||||
||||Start heap/byte|53000|||Start heap/byte|49184|||Start heap/byte||51824|
|||||||||||||||
||Fucntion||Stack/byte|Heap/byte|Heap cost/byte||Stack/byte|Heap/byte|Heap cost/byte||Stack/byte|Heap/byte|Heap cost/byte|
||isalnum||96|53000|0||80|49184|0||80|51824|0|
||isalpha||80|53000|0||64|49184|0||64|51824|0|
||isspace||80|53000|0||64|49184|0||64|51824|0|
|||||||||||||||
||atoi||160|53000|0||144|49184|0||144|51824|0|
||strtol||144|53000|0||128|49184|0||128|51824|0|
||atof||224|53000|0||208|49184|0||208|51824|0|
||atoff||224|53000|0||208|49184|0||208|51824|0|
||strtod||208|53000|0||192|49184|0||192|51824|0|
||strtof||208|53000|0||192|49184|0||192|51824|0|
|||||||||||||||
||asprintf||880|52616|384||1200|48800|384||608|51632|192|
||sscanf ||1552|53000|0||2032|49184|0||1120|51824|0|
|||||||||||||||
||gmtime||944|52616|384||1040|48576|608||1296|51512|312|
||localtime||160|53000|0||288|49184|0||288|51824|0|
||mktime||320|53000|0||448|49184|0||448|51824|0|
||asctime||768|52488|480||1264|48528|656||752|51464|360|
||settimeofday||336|53000|0||320|49184|0||320|51824|0|
||gettimeofday||480|53000|0||464|49184|0||464|51824|0|
||localtime_r||304|53000|0||432|49184|0||432|51824|0|
|||||||||||||||
||asprintf（64bit）||752|52424|576||1136|48336|848||608|51272|552|
||sscanf（64bit）||1056|53000|0||1376|49184|0||1040|51824|0|
