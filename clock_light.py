# Light Control Script by Chris Kuzma
#
# Designed as an alarm-clock of sorts to toggle a lamp attached to a relay
# circuit being controlled by an Arduino Nano. It relies heavily on the
# built-in features of a Unix computer and thus will not work on Windows,
# at least not at this point.
#
# How to use as-is:
# -Change the time zone to the appropriate one for your location
# -Upload "light_server.ino" to the Arduino and attach the ESP8266 as described
# -Attach the relay / LED you want to toggle to pin 2 of the Arduino
# -Run this script, setting on and off times as prompted (must follow syntax)
#


from datetime import datetime
from dateutil import tz
import time
import os

alarmTime = raw_input('Please enter in an alarm time (e.g. 08:00:00): ')
lightOffTime = raw_input('Please enter in a light-off time (e.g. 08:30:00): ')
lightStatus = 'Off'

from_zone = tz.gettz('UTC')
to_zone = tz.gettz('America/Indiana/Indianapolis') # Edit this to your own timezone

x = 0
while x < 1:
    utc = datetime.utcnow()
    utc = utc.replace(tzinfo=from_zone)
    utc = utc.astimezone(to_zone)
    utc = str(utc).split(' ')
    utc = utc[1].split('.')
    utc = utc[0]
    if utc == alarmTime:
        os.system('curl -m 1 http://LOCAL-IP/?command=10E') # This turns my light on
        lightStatus = 'On'
    if utc == lightOffTime:
        os.system('curl -m 1 http://LOCAL-IP/?command=11E') # This turns my light off
        lightStatus = 'Off'
    print 'Current Time\t' + utc
    print 'Alarm Time\t' + alarmTime
    print 'Off Time\t' + lightOffTime
    print 'Light Status\t' + lightStatus
    time.sleep(1)
    os.system('clear')
