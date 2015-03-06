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
# -Modify IP address to the correct one
# -Run this script

from datetime import datetime
from dateutil import tz
import time
import os
import sys

if len(sys.argv) != 3:
    print 'ERROR: Not enough parameters given.'
    print '\nUsage:\t\tpython ' + sys.argv[0] + ' on off'
    print 'Example:\tpython ' + sys.argv[0] + ' 08:00:00 08:15:00'
    print '\n\tTime format (24h): hh:mm:ss\n\n'
    exit()

alarmTime = sys.argv[1]
lightOffTime = sys.argv[2]
lightStatus = 'Off'

from_zone = tz.gettz('UTC')
to_zone = tz.gettz('America/Indiana/Indianapolis')

x = 0
while x < 1:
    utc = datetime.utcnow()
    utc = utc.replace(tzinfo=from_zone)
    utc = utc.astimezone(to_zone)
    utc = str(utc).split(' ')
    utc = utc[1].split('.')
    utc = utc[0]
    if utc == alarmTime:
        os.system('curl -m 10 http://192.168.1.67/?command=0E')
        lightStatus = 'On'
    if utc == lightOffTime:
        os.system('curl -m 10 http://192.168.1.67/?command=1E')
        lightStatus = 'Off'
    print 'Current Time\t' + utc
    print 'Alarm Time\t' + alarmTime
    print 'Off Time\t' + lightOffTime
    print 'Light Status\t' + lightStatus
    time.sleep(1)
    os.system('clear')
