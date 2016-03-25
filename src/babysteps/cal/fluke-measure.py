#!/usr/bin/env python

from fluke8845a import fluke8845a
import urllib2, json, time
from getchar import getchar

fname = time.strftime("%y%m%d-caldata.csv",time.localtime(time.time()))
fdata = open(fname,"a")


meter = fluke8845a()

running = True

while running:
    pvv = meter.voltage
    pvi = -meter.current

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

fdata.close()
            
            
