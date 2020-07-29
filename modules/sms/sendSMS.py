import serial
from curses import ascii
import sys
import time

if len(sys.argv)!=3:
    print('Usage:')
    print('\tpython sendSMS.py <phonenumber> <message>')
    sys.exit(1)


phonenumber = sys.argv[1]
SMS = sys.argv[2]
ser = serial.Serial('/dev/ttyUSB0', 460800, timeout=1)
# 460800 is baud rate, ttyUSB0 is virtual serial port we are sending to
ser.write("AT\r\n")
# send AT to the ttyUSB0 virtual serial port
line = ser.readline()
print(line)
# what did we get back from AT command? Should be OK
ser.write("AT+CMGF=1\r\n")
# send AT+CMGF=1 so setting up for SMS followed by CR 
line = ser.readline()
print(line)
# what did we get back from that AT command?
ser.write('AT+CMGS="%s"\r\n' %phonenumber)
# send AT+CMGS then CR, then phonenumber variable
ser.write(SMS)
# send the SMS variable after we sent the CR
ser.write(ascii.ctrl('z'))
# send a CTRL-Z after the SMS variable using ascii library
time.sleep(10)
# wait 10 seconds
print(ser.readline())
print(ser.readline())
print(ser.readline())
