import time, numpy as np
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtWidgets

# Parameters from lab 4
mass = 1360.0
Crr = 0.02
Cd  = 0.5
A_frontal   = 2.0
rw  = 0.28
theta = np.deg2rad(10)     # try 0 first to sanity-check
vw = 2.0
gear = 1.65                 # <-- reduction ratio (motor:wheel)
g = 9.81
dt = 0.001

# *** Frequencies (in Hz): ***
# Simulation and MCU frequency:
f_sim = 1000
# GUI Update frequency:
f_GUI = 50

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

# Update graph function:
def update():
  steps = f_sim / f_GUI
  torque_cmd = 0.0
  for x in range(int(steps)):
    # Example torque pattern (2 s on / 2 s off)
    
    if ((time.perf_counter() % 4) < 2):
      torque_cmd = 2000.0
    else:
        torque_cmd = 0.0

    # Wheel drive force (with 1.65 Gear reduction)
    F_wheels = gear * torque_cmd / rw

    # Opposing forces:
    F_drag = 0.5 * Cd * A_frontal * (speed_hist[-1] + vw)**2
    F_grav_x = mass * g * np.sin(theta)
    F_rr = Crr * mass * g * np.cos(theta)

    # Calculate next values
    a_next = (F_wheels - (F_drag + F_grav_x + F_rr)) / mass
    # Limit to forward driving only
    v_next = max(0.0, speed_hist[-1] + a_next * dt)
    w_next = v_next / rw

  # Plot and update buffers
  t_next = time.perf_counter() - t0
  t_hist.append(t_next)
  Torque_hist.append(torque_cmd)
  speed_hist.append(v_next)
  w_hist.append(w_next)
  # Keep array size below max
  if len(t_hist) > max_pts:
    t_hist.pop(0)
    Torque_hist.pop(0)
    speed_hist.pop(0)
    w_hist.pop(0)

  torque_curve.setData(t_hist, Torque_hist)
  speed_curve.setData(t_hist, speed_hist)
  wheelspeed_curve.setData(t_hist, w_hist)

# Start Simulation
timer = QtCore.QTimer()
timer.timeout.connect(update)
timer.start(20)
QtWidgets.QApplication.instance().exec()

