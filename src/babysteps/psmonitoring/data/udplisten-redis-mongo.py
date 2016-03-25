#!/usr/bin/env python

import select, socket, json, time, redis

port = 12345  # where do you expect to get a msg?
bufferSize = 1024 # whatever you need

r = redis.Redis("t")

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('192.168.0.255', port))
s.setblocking(0)

while True:
    result = select.select([s],[],[])
    msg = result[0][0].recv(bufferSize) 
    #print msg
    d = json.loads(msg)
    id = "%s.%s"%(d['class'],d['ipaddress'].split(".")[3])
    for k in d.keys():
        idk = "%s.%s"%(id,k) 
        r.setex(idk,d[k],120)
    data = "%s,%f,%s,%d,%d,%d,%d,%s"%(time.strftime("%y%m%d-%H%M%S",time.localtime(time.time())),time.time(),d['ipaddress'],d['adc0'],d['adc1'],d['adc2'],d['adc3'],d['state'])
    print data
    
