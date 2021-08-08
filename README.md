# WacomRipOff_Teensy
WacomRipOff project ported on Teensy++ 2.0  
ported from WacomRipOff project [here](https://github.com/TiNredmc/WacomRipoff)  


Most of this code is working in the same manner likes STM32, except that instead of using interrupt (which is really buggy here in Arduino). I workaround-ed with the polling instead, but the outcome is no different.
= 

# USB HID Core update
Some part of the Teensy USB HID need to be updated in order to have the custom USBHID report descriptor. all information is in the file called "USBHID.txt"
