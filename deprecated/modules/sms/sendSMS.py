#use python2 to run
import serial
from curses import ascii
import sys
import time

# define sendsms function
def sendSMS(port, phonenumber, sms):
    #define serial
    ser = serial.Serial(port, 460800, timeout=1)
    # 460800 is baud rate, ttyUSB0 is virtual serial port we are sending to
    
    # send AT to the ttyUSB0 virtual serial port
    ser.write("AT\r\n")
    
    # show AT response status
    print(ser.readline())
    
    # send AT+CMGF=1 so setting up for SMS followed by CR 
    ser.write("AT+CMGF=1\r\n")
    print(ser.readline())
    # send AT+CMGS then CR, then phonenumber variable
    ser.write('AT+CMGS="%s"\r\n' %phonenumber)

    # send the SMS variable after we sent the CR
    ser.write(SMS)

    # wait 3 seconds
    time.sleep(3)
    # send a CTRL-Z after the SMS variable using ascii library
    ser.write(ascii.ctrl('z'))
    
    # wait 5 seconds
    time.sleep(5)
    
    print(ser.readline())
    print(ser.readline())
    print(ser.readline())

#--------main--------


# check args
try:
    if len(sys.argv)==4:
        port = sys.argv[1]
        phonenumber = sys.argv[2]
        SMS = sys.argv[3]
        
        sendSMS(port, phonenumber, SMS)
    else:
        print('Usage:')
        print('\tpython sendSMS.py <serial port> <phonenumber> <message>')
        sys.exit(1)
except Exception as ex:
    print(ex)



