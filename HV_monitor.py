import numpy as np
import zmq
import ctypes
import sys
import threading
import datetime

PRINT=True
PLOT=False
SAVE=True

DIR = '/data/1/WbLS-DATA/phase_3/HV/'

if PLOT:
	import matplotlib.pyplot as plt
	from matplotlib.animation import FuncAnimation
	bd_status = list(np.zeros(30))
	temps = list(np.zeros(30))
	ch_status = list(np.zeros(30))
	ch_vmon = list(np.zeros(30))
	ch_imon = list(np.zeros(30))
	ch_pw = list(np.zeros(30))

class HVData(ctypes.Structure):
	'''Matches EosDAQ's HVData struct'''
	class HVChData(ctypes.Structure):
		_fields_ = [
			('chID', ctypes.c_int),
			('VMon', ctypes.c_float),
			('IMon', ctypes.c_float),
			('Status', ctypes.c_uint32),
			('Pw', ctypes.c_bool),
		]
	_fields_ = [
		('slot', ctypes.c_int),
		('Temp', ctypes.c_float),
		('BdStatus', ctypes.c_uint32),
		('channels', HVChData * 32),
	]

def listen():
	'''Listen to the ZMQ dispatch stream and update the data.'''
	c = zmq.Context(1)
	s = c.socket(zmq.SUB)
	s.setsockopt_string(zmq.SUBSCRIBE, "")
	s.connect('tcp://130.199.33.252:5432')

	global bd_status
	global temps
	global ch_status
	global ch_vmon
	global ch_imon
	global ch_pw

	while True:
		if SAVE:
			HVInfo = []
			TimeStamp = None
			event_counter = 0
		while True:
			r = s.recv()
			if sys.getsizeof(r)==65:
				print("Socket disconnected")
				break
			d = HVData.from_buffer_copy(r)

			now = datetime.datetime.now()
			timestamp = now.strftime('%y%m%dT%H%M')
			if TimeStamp==None:
				TimeStamp = timestamp

			if PRINT:
				slot = d.slot
				print("board: ", slot)
				print("temp: ", d.Temp)
				print(d.BdStatus)
				print("Channels")
				for i, ch in enumerate(d.channels):
					print("ChID: ", ch.chID)
					print("VMon: ", ch.VMon)
					print("IMon: ", ch.IMon)
					print("Pw: ", ch.Pw)
					print("Status: ", ch.Status)

			if (PLOT and d.slot==1):
				bd_status.append(d.BdStatus)
				bd_status.pop(0)

				temps.append(d.Temp)
				temps.pop(0)

				ch_status.append(d.channels[0].Status)
				ch_status.pop(0)

				ch_vmon.append(d.channels[0].VMon)
				ch_vmon.pop(0)

				ch_imon.append(d.channels[0].IMon)
				ch_imon.pop(0)

				ch_pw.append(d.channels[0].Pw)
				ch_pw.pop(0)

			if SAVE:
				
				anHVInfo = {}
				anHVInfo['Timestamp'] = timestamp
				anHVInfo['Slot'] = d.slot
				anHVInfo['Temp'] = d.Temp
				anHVInfo['BdStatus'] = d.BdStatus
				channels = {}
				for i, ch in enumerate(d.channels):
					chID = 'ch' + str(ch.chID)
					channel = {}
					channel['VMon'] = ch.VMon
					channel['IMon'] = ch.IMon
					channel['Pw'] = ch.Pw
					channel['Status'] = ch.Status
					channels[chID] = channel
				anHVInfo['Channels'] = channels
				HVInfo.append(anHVInfo)
				event_counter += 1
				if (event_counter > 1500):
					fname = 'HV_Monitoring_' + TimeStamp
					np.save(DIR + fname, HVInfo, allow_pickle=True)
					TimeStamp = None
					HVInfo = []
					event_counter = 0

				

		if SAVE:
			fname = 'HV_Monitoring_' + TimeStamp
			np.save(DIR + fname, HVInfo, allow_pickle=True)
def plot():
	fig, axs = plt.subplots(3, 2)
	axs = axs.flatten()

	def plot_fcn(i):
		for ax in axs:
			ax.cla()

		axs[0].plot(bd_status)
		axs[0].set_title('Board 1 Status')

		axs[1].plot(temps)
		axs[1].set_title('Board Temp')
		axs[1].set_ylabel('C')

		axs[2].plot(ch_pw)
		axs[2].set_title('Board 1 Ch 0 Power')

		axs[3].plot(ch_status)
		axs[3].set_title('Board 1 Ch 0 Status')

		axs[4].plot(ch_vmon)
		axs[4].set_title('Board 1 Ch 0 Voltage')
		axs[4].set_ylabel('V')

		axs[5].plot(ch_imon)
		axs[5].set_title('Board 1 Ch 0 Current')
		axs[5].set_ylabel('A')

	ani = FuncAnimation(fig, plot_fcn, interval=500)

	plt.show()


if __name__ == '__main__':
	listener = threading.Thread(target=listen)
	listener.start()

	if PLOT: plot()
