import time, numpy as np
import pyqtgraph as pg
import struct
import time

# Graphing Library
from pyqtgraph.Qt import QtCore, QtWidgets
# Library for threads
QMetaObject = QtCore.QMetaObject
import threading
import socket

# Setting up the plotting thread updating class
class CurveUpdater(QtCore.QObject):
    update_curve = QtCore.pyqtSignal(object, object)  # x and y data

    def __init__(self, curve_item):
        super().__init__()
        self.curve = curve_item
        self.update_curve.connect(self._update)

    @QtCore.pyqtSlot(object, object)
    def _update(self, x, y):
        self.curve.setData(x, y)  # SAFE: runs in GUI thread

# SETTING UP BOARD TO COMPUTER COMMUNICATION CHANNEL
esp32_ip = "192.168.137.134"  #NOTE 2 SELF: MAY need to change if network changes
#esp32_ip = "192.168.2.x"

esp32_port = 6000 # (i.e. sendPort)           

receivingSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
receivingSock.bind(("0.0.0.0", 5005))

sendingSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print("Listening on Laptop UDP port 5005 and sending to ESP32 at", esp32_ip)

# "Global" Variables:
Current_Torque = 0
Current_Speed = 0
Current_w = 0

lastTime = time.time()      # seconds since UNIX epoch (float)
time.sleep(1.0)      # sleeps for 1 second
# Functions:

# Translating from Board value to real value
translation_factor = 0.01 # approximately equal to (max-min) / 2^16 - Torque definition from board

# Parameters from lab 4
mass = 1360.0
Crr = 0.02
Cd  = 0.5
A_frontal   = 2.0
rw  = 0.28
theta = np.deg2rad(10) # try 0 first to sanity-check
vw = 2.0
gear = 9 #1.65 # reduction ratio (motor:wheel)
g = 9.81
dt = 0.001

# *** Frequencies (in Hz): ***
 # ****************** JACK + MARZOUK UPDATE HERE ********************************
 # We need to decide on how often data is sent back and forth from sim to board
 # We should draw a timing diagram or something to time the waits in both codes
# How many new torque values before GUI updates
inputs_per_graph_update = 10

# *** GUI Code: ***

app = QtWidgets.QApplication([])
win = pg.GraphicsLayoutWidget(show=True, title="EV Simulation 1 kHz")

# Input Torque Plot:
p1 = win.addPlot(title="Torque Command [Nm]"); p1.showGrid(x=True,y=True)
torque_curve = p1.plot(pen=pg.mkPen('r'), name="Torque")
torque_updater = CurveUpdater(torque_curve)
# Speed Plot:
p2 = win.addPlot(title="Vehicle & Wheel Speed"); p2.showGrid(x=True,y=True)
p2.addLegend()
speed_curve = p2.plot(pen=pg.mkPen('g'), name="v [m/s]") 
speed_updater = CurveUpdater(speed_curve)
wheelspeed_curve = p2.plot(pen=pg.mkPen('b'), name="w [rad/s]")
wheelspeed_updater = CurveUpdater(wheelspeed_curve)

# Create buffers:
max_pts = 1000
t0 = time.perf_counter()
t_hist, Torque_hist, speed_hist, w_hist = [], [], [], []
speed_hist.append(0)
Torque_hist.append(0)
t_hist.append(0)
w_hist.append(0)

# When receiving torque from board:
def on_new_data(Torque):
  global Current_Torque, Current_Speed, Current_w
  # 1. Update the current torque variable
  Current_Torque = (Torque * translation_factor) - 150

  print("Current Torque:", Current_Torque)

  # 2. Calculate acceleration and speed

  # Wheel drive force (with 1.65 Gear reduction)
  F_wheels = gear * Current_Torque / rw
  print("\nF_wheels (N):\n", F_wheels)

  # Opposing forces:
  F_drag = 0.5 * Cd * A_frontal * (Current_Speed + vw)**2
  print("\nF_drag (N):\n", F_drag)
  F_grav_x = mass * g * np.sin(theta)
  print("\nF_grav_x (N):\n", F_grav_x)
  F_rr = Crr * mass * g * np.cos(theta)
  print("\nF_rr:\n", F_rr)
  global lastTime
  timeNow = time.time()
  deltaTime = (timeNow - lastTime)
  print("\nDelta Time (s):\n", deltaTime)
  lastTime = timeNow     # seconds since UNIX epoch (float)

  # Calculate next values
  a_next = (F_wheels - (F_drag + F_grav_x + F_rr)) / mass
  print("\n1Current Accel (m/s^2):\n", a_next)
  # Limit to forward driving only
  Current_Speed = max(0.0, Current_Speed + a_next * deltaTime)
  Current_w = Current_Speed / rw

  print("\n2Current Speed (m/s):\n", Current_Speed)

  #Translate speed to km/h
  speed_kmh = Current_Speed / 3.6

  send_to_board = int(round(speed_kmh * 2))
  # 3. Send speed to board
  #SEND DATA BACK
  outboundMessage = struct.pack("i", send_to_board)
  sendingSock.sendto(outboundMessage, (esp32_ip, esp32_port))
  #outboundMessage = int(send_to_board).encode()
  #sendingSock.sendto(outboundMessage, (esp32_ip, esp32_port))
  print("Sent to ESP32:", send_to_board)

# ******** MAIN EVENT LOOP: **********
def eventLoop():
  global Current_Torque, Current_Speed, Current_w
  while (1):
    count = 0
    while (count < inputs_per_graph_update):
      data, addr = receivingSock.recvfrom(1024) # Blocks until received Torque command
      valueReceived = int(data.decode())
      print("Received from ESP32:", valueReceived)
      count += 1
      on_new_data(valueReceived) # ********************* UPDATE TO PASS ACTUAL INCOMING TORQUE *************
    
    # When inputs_per_graph_update cycles have gone by, update the graph

    # a) Update buffers
    t_next = time.perf_counter() - t0
    t_hist.append(t_next)
    Torque_hist.append(Current_Torque)
    speed_hist.append(Current_Speed)
    w_hist.append(Current_w)

    # b) Keep array size below max
    if len(t_hist) > max_pts:
      t_hist.pop(0)
      Torque_hist.pop(0)
      speed_hist.pop(0)
      w_hist.pop(0)

    # c) Plot buffers
    # Invoke the methods from the plotting thread:
    # QMetaObject.invokeMethod(torque_curve, "setData", QtCore.Qt.QueuedConnection, QtCore.Q_ARG(object,t_hist), QtCore.Q_ARG(object,Torque_hist))
    torque_updater.update_curve.emit(t_hist, Torque_hist)
    # QMetaObject.invokeMethod(speed_curve, "setData", QtCore.Qt.QueuedConnection, QtCore.Q_ARG(object,t_hist), QtCore.Q_ARG(object,speed_hist))
    speed_updater.update_curve.emit(t_hist, speed_hist)
    # QMetaObject.invokeMethod(wheelspeed_curve, "setData", QtCore.Qt.QueuedConnection, QtCore.Q_ARG(object,t_hist), QtCore.Q_ARG(object,w_hist))
    wheelspeed_updater.update_curve.emit(t_hist, w_hist)
    # torque_curve.setData(t_hist, Torque_hist)
    # speed_curve.setData(t_hist, speed_hist)
    # wheelspeed_curve.setData(t_hist, w_hist)



# Run Event loop
loopthead = threading.Thread(target=eventLoop, daemon=True)
loopthead.start()
QtWidgets.QApplication.instance().exec()
