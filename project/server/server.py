#! /usr/bin/python
# -*- coding: utf-8 -*-
from pyfirmata import Arduino, util
from socket import *
from select import *
import sys
import time
import tty
import termios

def getkey():
    fd =sys.stdin.fileno()
    original_attributes = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd,termios.TCSADRAIN, original_attributes)
    return ch

def steering(cmdtxt):
	if(cmdtxt[3] == 76 and cmdtxt[4] == 76):	#LL
		pin_motor.write(0.1)
	elif(cmdtxt[3] == 76 and cmdtxt[4] == 77):	#LM
		pin_motor.write(0.2)
	elif(cmdtxt[3] == 76 and cmdtxt[4] == 82):	#LR
		pin_motor.write(0.4)
	elif(cmdtxt[3] == 83 and cmdtxt[4] == 83):	#SS
		pin_motor.write(0.5)
	elif(cmdtxt[3] == 82 and cmdtxt[4] == 76):	#RL
		pin_motor.write(0.6)
	elif(cmdtxt[3] == 82 and cmdtxt[4] == 77):	#RM
		pin_motor.write(0.8)
	elif(cmdtxt[3] == 82 and cmdtxt[4] == 82):	#RR
		pin_motor.write(1)
	print(cmdtxt)

board = Arduino('/dev/ttyACM0')
pin_motor = board.get_pin('d:6:p')

it = util.Iterator(board)
it.start()

HOST = ''
PORT = 7777
BUFSIZE = 1024
ADDR = (HOST,PORT)

serverSocket = socket(AF_INET, SOCK_STREAM)#1.소켓을 생성한다.

serverSocket.bind(ADDR) #2.소켓 주소 정보 할당

print('bind')

while True:
	try:
		serverSocket.listen(0) #3.연결 수신 대기 상태

		print('listen')

		clientSocket, addr_info = serverSocket.accept() #4.연결 수락

		print('accept')

		cmdtxt = clientSocket.recv(BUFSIZE)

		steering(cmdtxt)

		time.sleep(0.01)
		clientSocket.close() #소켓 종료

	except KeyboardInterrupt:
		key = getkey()
		if key == 'c':
			print("\n")
			clientSocket.close() #소켓 종료
			break

serverSocket.close()

print('close')
#sys.exit(1)