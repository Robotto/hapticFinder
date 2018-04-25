#!/usr/bin/python
import termios, fcntl, sys, os
import socket

fd = sys.stdin.fileno()

oldterm = termios.tcgetattr(fd)
newattr = termios.tcgetattr(fd)
newattr[3] = newattr[3] & ~termios.ICANON & ~termios.ECHO
termios.tcsetattr(fd, termios.TCSANOW, newattr)

oldflags = fcntl.fcntl(fd, fcntl.F_GETFL)
fcntl.fcntl(fd, fcntl.F_SETFL, oldflags | os.O_NONBLOCK)

UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
remotePort = 1337
remoteIP = '192.168.43.139'

print
print '     ===== Goodmorning, you sexy hackers!  ====='
print '     = Use arrow keys to send movement signals ='
print '     =  (pgup / pgdn to send forward / back)   ='
print '     ==========================================='
print

try:
    while 1:
        try:
            c = sys.stdin.read(1)
 
            if c=='\x1b': #arrow keys: UP='A', DOWN='B', RIGHT='C', LEFT='D' 
                sys.stdin.read(1)
                c=sys.stdin.read(1)
                #if c=='5' or c=='6': #sideeffect of pgup and pgdn
                #	sys.stdin.read(1)
            
            if c=='A':
            	print 'up'
            	tx='U'
            elif c=='B':
            	print 'Down'
            	tx='D'
            elif c=='C':
            	print 'Right'
            	tx='R'
            elif c=='D':
            	print 'Left'
            	tx='L'
            elif c=='5':
            	print 'Forward'
            	tx='F'
            	sys.stdin.read(1) #sideeffect of pgup and pgdn
            elif c=='6':
            	print 'Back'
            	tx='B'
            	sys.stdin.read(1) #sideeffect of pgup and pgdn
            else:
            	print c
            	tx=c

            UDPSock.sendto(tx, (remoteIP, remotePort))

            #print "Got character", repr(c)
        except IOError: pass
finally:
    termios.tcsetattr(fd, termios.TCSAFLUSH, oldterm)
    fcntl.fcntl(fd, fcntl.F_SETFL, oldflags)