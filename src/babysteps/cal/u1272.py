#!/usr/bin/env python

# Python script to communicate with the
# Agilent U1253A / U1272A / U1273A etc.
# found originally on http://goo.gl/Gycv9H

# For more information on the protocol, check
# http://blog.philippklaus.de/2014/02/agilent-u1273a/
# and http://goo.gl/oIJi96

import sys
import time
import serial

class u1272:
    def __init__(self,com_port,baud=9600):
        self.port = serial.Serial(com_port, baud, timeout=.1)

    def idn(self):
        self.port.write("*IDN?\n")
        return self.port.read(100).strip()

    def read(self,second='no'):
        if second == 'yes':
            self.port.write("FETC? @2\n")
        else:
            self.port.write("FETC?\n")
        rv = self.port.read(17)
        rv = float(rv)
        return rv

    def close(self):
        self.port.close()

if __name__=='__main__':
    try:
        port = sys.argv[1]
        baud = int(sys.argv[2])
    except:
        port = "/dev/ttyUSB0"
        baud = 9600

    dmm = u1272(port,baud)
    print dmm.read()
