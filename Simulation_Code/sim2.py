import time, numpy as np
import pyqtgraph as pg
# Graphing Library
from pyqtgraph.Qt import QtCore, QtWidgets
# Library for threads
QMetaObject = QtCore.QMetaObject
import threading

# "Global" Variables:
Current_Torque = 0
Current_Speed = 0
Current_w = 0

# Functions:


# Parameters from lab 4
mass = 1360.0
Crr = 0.02
Cd  = 0.5
A_frontal   = 2.0
rw  = 0.28
theta = np.deg2rad(10) # try 0 first to sanity-check
vw = 2.0
gear = 1.65 # reduction ratio (motor:wheel)
g = 9.81
dt = 0.001

# *** Frequencies (in Hz): ***
 # ****************** JACK + MARZOUK UPDATE HERE ********************************
 # We need to decide on how often data is sent back and forth from sim to board
 # We should draw a timing diagram or something to time the waits in both codes
# How many new torque values before GUI updates
inputs_per_graph_update = 50

# *** GUI Code: ***

app = QtWidgets.QApplication([])
win = pg.GraphicsLayoutWidget(show=True, title="EV Simulation 1 kHz")

# Input Torque Plot:
p1 = win.addPlot(title="Torque Command [Nm]"); p1.showGrid(x=True,y=True)
torque_curve = p1.plot(pen=pg.mkPen('r'), name="Torque")
# Speed Plot:
p2 = win.addPlot(title="Vehicle & Wheel Speed"); p2.showGrid(x=True,y=True)
p2.addLegend()
speed_curve = p2.plot(pen=pg.mkPen('g'), name="v [m/s]") 
wheelspeed_curve = p2.plot(pen=pg.mkPen('b'), name="w [rad/s]")

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
  Current_Torque = Torque

  # 2. Calculate acceleration and speed

  # Wheel drive force (with 1.65 Gear reduction)
  F_wheels = gear * Current_Torque / rw

  # Opposing forces:
  F_drag = 0.5 * Cd * A_frontal * (Current_Speed + vw)**2
  F_grav_x = mass * g * np.sin(theta)
  F_rr = Crr * mass * g * np.cos(theta)

  # Calculate next values
  a_next = (F_wheels - (F_drag + F_grav_x + F_rr)) / mass
  # Limit to forward driving only
  Current_Speed = max(0.0, Current_Speed + a_next * dt)
  Current_w = Current_Speed / rw

  # 3. Send speed to board
  # ********************************* JACK + MARZOUK UPDATE HERE ***********************
  # Send speed to board on every new data cycle from the board

# ******** MAIN EVENT LOOP: **********
def eventLoop():
  global Current_Torque, Current_Speed, Current_w
  while (1):
    count = 0
    while (count < inputs_per_graph_update):
      while(1):
        # ****************************** JACK + MARZOUK UPDATE HERE ***********************
        # poll for new torque data
        1
        
      count += 1
      on_new_data(TORQUE) # ********************* UPDATE TO PASS ACTUAL INCOMING TORQUE *************
    
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
    QMetaObject.invokeMethod(torque_curve, "setData", QtCore.Qt.QueuedConnection, QtCore.Q_ARG(object,t_hist), QtCore.Q_ARG(object,Torque_hist))
    QMetaObject.invokeMethod(speed_curve, "setData", QtCore.Qt.QueuedConnection, QtCore.Q_ARG(object,t_hist), QtCore.Q_ARG(object,speed_hist))
    QMetaObject.invokeMethod(wheelspeed_curve, "setData", QtCore.Qt.QueuedConnection, QtCore.Q_ARG(object,t_hist), QtCore.Q_ARG(object,w_hist))
    # torque_curve.setData(t_hist, Torque_hist)
    # speed_curve.setData(t_hist, speed_hist)
    # wheelspeed_curve.setData(t_hist, w_hist)



# Run Event loop
loopthead = threading.Thread(target=eventLoop, daemon=True)
loopthead.start()
QtWidgets.QApplication.instance().exec()