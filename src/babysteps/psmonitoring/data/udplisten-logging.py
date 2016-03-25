#!/usr/bin/env python

import select, socket, json, time

port = 12345  # where do you expect to get a msg?
bufferSize = 1024 # whatever you need

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(('192.168.0.255', port))
s.setblocking(0)

fn = time.strftime("%y%m%d-data.csv",time.localtime(time.time()))
f = open(fn,"a")

while True:
    result = select.select([s],[],[])
    msg = result[0][0].recv(bufferSize) 
    d = json.loads(msg)
    data = "%s,%f,%s,%d,%d,%d,%d,%s"%(time.strftime("%y%m%d-%H%M%S",time.localtime(time.time())),time.time(),d['ipaddress'],d['adc0'],d['adc1'],d['adc2'],d['adc3'],d['state'])
    print data
    f.write(data+"\n")
    f.flush()
    
