#!/usr/bin/env python

import urllib2, json, time
from getchar import getchar
import time, u1272, sys, os

voltmeter = u1272.u1272('/dev/ttyUSB1',19200)
ammeter   = u1272.u1272('/dev/ttyUSB2',19200)

fname = time.strftime("%y%m%d-caldata.csv",time.localtime(time.time()))
fdata = open(fname,"a")

fdata.write("\n")

running = True

while running:
    pvilast = 0.0
    count = 0
    while True: # loop until value stops changing
        pvv = voltmeter.read()
        pvi = ammeter.read()

        if abs(pvilast - pvi) < 0.01:
            count += 1
        if count > 2:
            print 
            break
        pvilast = pvi
        time.sleep(0.25)
        sys.stdout.write(".")
        sys.stdout.flush()
        c = getchar()
        if c:
            if c == "q":
                sys.exit(0)
                break

    data = urllib2.urlopen('http://192.168.0.128/data').read()
    data = json.loads(data)
    row = "%s,%d,%.6f,%d,%.6f"%(time.strftime("%y%m%d-%H%M%S",time.localtime(time.time())),data['adc.pv.voltage'],pvv,data['adc.pv.current'],pvi)
    print row
    fdata.write(row+"\n")

    os.system('mpg123 -q ~/r/c/cellphone/ringtones/snare.mp3 & 2>&1')

    time.sleep(0.5)
    pvilast = ammeter.read()
    # wait here until value changes
    while abs(pvilast - pvi) < 0.1:
        pvi = ammeter.read()
        sys.stdout.write("w")
        sys.stdout.flush()
        time.sleep(0.25)
        c = getchar()
        if c:
            if c == "q":
                running = False
                break


fdata.close()

'''
while running:
    pvv = voltmeter.read()
    pvi = ammeter.read()

    data = urllib2.urlopen('http://192.168.0.128/data').read()
    data = json.loads(data)
    row = "%s,%d,%.6f,%d,%.6f"%(time.strftime("%y%m%d-%H%M%S",time.localtime(time.time())),data['adc.pv.voltage'],pvv,data['adc.pv.current'],pvi)
    print row
    fdata.write(row+"\n")
    while running:
        c = getchar()
        if c:
            if c == "q":
                running = False
            if c == " ":
                break
            if c == "b":
                fdata.write("\n")
                print 
'''
