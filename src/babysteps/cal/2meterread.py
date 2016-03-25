#!/usr/bin/env python

import time, u1272
import sys, os,termios

def getchar():
    '''
    Equivale al comando getchar() di C
    '''

    fd = sys.stdin.fileno()
    
    if os.isatty(fd):
        
        old = termios.tcgetattr(fd)
        new = termios.tcgetattr(fd)
        new[3] = new[3] & ~termios.ICANON & ~termios.ECHO
        new[6] [termios.VMIN] = 1
        new[6] [termios.VTIME] = 0
        
        try:
            termios.tcsetattr(fd, termios.TCSANOW, new)
            termios.tcsendbreak(fd,0)
            ch = os.read(fd,7)

        finally:
            termios.tcsetattr(fd, termios.TCSAFLUSH, old)
    else:
        ch = os.read(fd,7)
    
    return(ch)


v = u1272.u1272('/dev/ttyUSB1',19200)
i = u1272.u1272('/dev/ttyUSB2',19200)

while True:
    print "%f,%f"%(v.read(),i.read())
    k = getchar()
    if k == 'x':
        break 
