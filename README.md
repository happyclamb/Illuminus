#Illuminus


##Arduino Uploading
[Arduino Software](https://www.arduino.cc/download_handler.php)

####Add Arduino 328p Board:

`Tools -> Board -> Board Manager`

* Atmel AVR Xplained-minis by Atmel University France version v0.3.0

####Extra libraries:
	
`Sketch -> Include Library -> Manage Libraries`

* FastLED by Daniel Garcia; Version v3.1.0
* RF24 by TMRh20 Version v1.1.6


##Coding Environment

Atom for development:
[Atom](https://atom.io/)

After install, go to `File; Add Project File` and point it to the unzipped location of the illuminus code.

Finally; to config Atom to know about the arduino 'ino' files you need to add this to the `File; Config...`


>"*":  
&nbsp;&nbsp;&nbsp;core:  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;customFileTypes:  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"source.cpp": [  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"h"  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"ino"  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"cpp"  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;]  

