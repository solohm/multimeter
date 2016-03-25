#!/usr/bin/env python

import socket,time,serial,sys

def removeexp(s):
    try:
        return float(s)
    except ValueError:
        return s

class fluke8845a(object):
    def __init__(self,host='fluke1',port=3490):
        self.host = host
        self.port = port
        self.connect()

    def connect(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.settimeout(2.0)
        while True:
            try:
                self.socket.connect((self.host,self.port))
                print "opened fluke8845a",self.host,self.port
                self.clear()
                self.remote = 1
                self.command("*cls")
                self.command("zero:auto 0")
                self.command("syst:rem")
                self.command("volt:filt on")
                self.command("curr:filt on")
                if self.host != 'fluke1': # old firmware on fluke1, probably error check this instead of using fluke1 hostname
                    self.command("filt:dig on")
                    self.command("volt:filt:dig on")
                    self.command("curr:filt:dig on")
                break
            except socket.error:
                print "socket error. retrying..."
                time.sleep(2)

    def disconnect(self):
        if self.socket:
            self.socket.close()

    def close(self):
        self.disconnect()

    def command(self,c):
        self.socket.send(c+"\n")

    def query(self,c):
        while True:
            try:
                self.socket.send(c+'\n')
                reply = ""
                while True:
                    c = self.socket.recv(1)
                    #print c,ord(c)
                    if c != '\r':
                        reply += c
                    if c == '\n':
                        break
                return removeexp(reply.strip())
            except socket.error:
                self.disconnect()
                time.sleep(2)
                self.connect()

    def voltage():
        def fget(self):
            return self.query("meas:volt:dc? MAX")
        return locals()
    voltage = property(**voltage())

    def current():
        def fget(self):
            return self.query("meas:curr:dc? 4")
        return locals()
    current = property(**current())

    def remote():
        def fset(self,value):
            if value == 1:
                self.command("syst:rem")
            else:
                self.command("syst:loc")
        return locals()
    remote = property(**remote())

    def clear(self):
        self.command("*cls")

    def error():
        def fget(self):
            return self.query("syst:err?")
        return locals()
    error = property(**error())
    

if __name__=='__main__':
    meter = fluke8845a()
    print meter.voltage

    for i in range(10):
        print meter.voltage
        print meter.error
    print meter.current

    meter.remote = 0
