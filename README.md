# 48434-Embedded-Software-Project

## Notes:
Store Tariff in flash

	- should be able to set tariffs via tower protocol
FTM to change state to dormant after 15 seconds

2 modes

	-Intermediate	
	-Basic
	
RTC - to track day time

PIT - to keep track of time metering

Input conditioning circuitry - Sems like we'll have to process it in software

SW1 is on Port D pin 0. Used to cycle between displays

## TODO:
Tower crashes on first run (or after building and flashing)

	-Probably has to do with writing the tariffs on start up.
	-There is an access error FSTAT[ACCERR] idk why lol


## Useful Links
http://www.electronics-tutorials.ws/accircuits/average-voltage.html

http://www.electronics-tutorials.ws/accircuits/rms-voltage.html
