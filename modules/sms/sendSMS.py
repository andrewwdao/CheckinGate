#use python2 to run
import serial
from curses import ascii
import sys
import time


def init():
    #define serial
    ser = serial.Serial('/dev/ttyUSB0', 460800, timeout=1)
    # 460800 is baud rate, ttyUSB0 is virtual serial port we are sending to
    
    # send AT to the ttyUSB0 virtual serial port
    ser.write("AT\r\n")
    
    # show AT response status
    print(ser.readline())
    
    # send AT+CMGF=1 so setting up for SMS followed by CR 
    ser.write("AT+CMGF=1\r\n")
    print(ser.readline())

# define sendsms function
def sendSMS(phonenumber, sms):
    # send AT+CMGS then CR, then phonenumber variable
    ser.write('AT+CMGS="%s"\r\n' %phonenumber)

    # send the SMS variable after we sent the CR
    ser.write(SMS)

    # send a CTRL-Z after the SMS variable using ascii library
    ser.write(ascii.ctrl('z'))
    
    # wait 5 seconds
    time.sleep(5)
    
    print(ser.readline())
    print(ser.readline())
    print(ser.readline())

#--------main--------
init()

# check args
try:
    if len(sys.argv)==3:
        phonenumber = sys.argv[1]
        SMS = sys.argv[2]
        sendSMS(phonenumber, SMS)
    else:
        print('Usage:')
        print('\tpython sendSMS.py <phonenumber> <message>')
        sys.exit(1)
except:
    print('Cannot send request! Please send request with format below:')
    print('\t-To send sms:\tpython sendSMS.py <phonenumber> <message>')




