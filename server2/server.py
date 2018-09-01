#! /usr/bin/python
# -*- coding: utf-8 -*-


#from PyQt5.QtWidgets import *
#from PyQt5.QtCore import *
#from PyQt5.QtGui import *
#from PyQt5 import *

from pyfirmata import Arduino, util
from socket import *
from select import *
import serial
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

def serialdata():
	global real_data

	data = ardu.readline()
	data_len = len(data)

	for i in range(data_len):
		if(data[i+4] == 10):
			real_data = []
			real_data.append(data[i])
			real_data.append(data[i+1])
			real_data.append(data[i+2])
			real_data = bytes(real_data)
			break

def steering(cmdtxt):
	if(cmdtxt[3] == 76 and cmdtxt[4] == 76):	#LL
		pin_motor.write(0.1)
	elif(cmdtxt[3] == 76 and cmdtxt[4] == 77):	#LM
		pin_motor.write(0.2)
	elif(cmdtxt[3] == 76 and cmdtxt[4] == 78):	#LN
		pin_motor.write(0.3)
	elif(cmdtxt[3] == 76 and cmdtxt[4] == 82):	#LR
		pin_motor.write(0.4)
	elif(cmdtxt[3] == 83 and cmdtxt[4] == 83):	#SS
		pin_motor.write(0.5)
	elif(cmdtxt[3] == 82 and cmdtxt[4] == 76):	#RL
		pin_motor.write(0.6)
	elif(cmdtxt[3] == 82 and cmdtxt[4] == 77):	#RM
		pin_motor.write(0.7)
	elif(cmdtxt[3] == 82 and cmdtxt[4] == 78):	#RN
		pin_motor.write(0.8)
	elif(cmdtxt[3] == 82 and cmdtxt[4] == 82):	#RR
		pin_motor.write(1)
	print(cmdtxt)

def motorBrake(cmdtxt):
	global stop_flag

	if (cmdtxt[5] == 79):			#stop
		if (stop_flag == 0):
			print("stop")
			clientSocket.sendto('stop'.encode(),addr_info)
			stop_flag = 1

			brake_brake.write(0)
			brake_dir.write(0)
			time.sleep(2.7)

			brake_dir.write(1)
			time.sleep(0.35)
			brake_brake.write(1)

	else:				#No stop
		clientSocket.sendto('go'.encode(),addr_info)


board = Arduino('/dev/ttyACM0')
#ardu = serial.Serial('/dev/ttyACM0', 9600)
pin_motor = board.get_pin('d:6:p')
brake_brake = board.get_pin('d:5:o')
brake_dir = board.get_pin('d:8:o')

brake_brake.write(1)

it = util.Iterator(board)
it.start()

HOST = ''
PORT = 7777
BUFSIZE = 1024
ADDR = (HOST,PORT)
real_data = [48, 48, 48]
real_data = bytes(real_data)
stop_flag = 0
stop_cnt = 0

serverSocket = socket(AF_INET, SOCK_STREAM)#1.소켓을 생성한다.

serverSocket.bind(ADDR) #2.소켓 주소 정보 할당

print('bind')

while True:
	try:
		serverSocket.listen(0) #3.연결 수신 대기 상태
		#print('socket listen')

		clientSocket, addr_info = serverSocket.accept() #4.연결 수락
		#print('socket accept')

		'''
		try:						#speed data
			serialdata()
			print('speed = ' + str(real_data.decode('utf-8')) + 'km/h')			#current speed
		except:
			print('speed = ' + str(real_data.decode('utf-8')) + 'km/h' + ' before')			#past speed
			print("Serial error")
		'''
		cmdtxt = clientSocket.recv(BUFSIZE)

		try:
			steering(cmdtxt)
		except:
			print("steering error")

		#try:
		motorBrake(cmdtxt)
		#except:
		#	print("brake error")

		if (stop_flag == 1):
			stop_cnt = stop_cnt + 1

		print("stop_flag : ")
		print(stop_flag)
		print("stop_cnt : ")
		print(stop_cnt)

		if (stop_cnt >= 100):
			stop_flag = 0
			stop_cnt = 0

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