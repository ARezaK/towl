My notes:
Used ESP8266 initially but for wifi range i used wemos d1 mini pro
and this module
https://www.ebay.com/itm/VK2828U7G5LF-Ublox-GPS-Module-FLASH-Flight-Controller-W-Antenna-TTL-1-10Hz/272786136744?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2060353.m2749.l2649

Installed base32 module from here https://github.com/NetRat/Base32 . Download zip and install library from zip using arduino software
Installed tinygps++ teh same way
and this https://github.com/PaulStoffregen/Time
I could only get this to work w/ esp8266 board version 2.3.0 (selected via board manager in arduino)
setup dnslogger-Poc on your server

Connect power (5V), and ground and Connect the TX pin of the GPS to the D6 pin of the ESP (the Blue wire)

Takes about 2 minutes for it to startup and you have to be close to a window.

If you have problems with the code may have to somtimes erase flash via
esptool.py -p COM6 erase_flash
OR
on mac
esptool.py -p /dev/cu.SLAB_USBtoUART erase_flash


Used this website to view lat long
https://www.darrinward.com/lat-long/

Use this to extract from log
cat a01.log | cut -d ',' -f3,4,5


setting up networking on digitalocean
https://gyazo.com/2d39fcdbe8c663b8ba61023c195601ae
