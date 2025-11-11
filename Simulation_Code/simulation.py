import time, numpy as np
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtWidgets

# Parameters from lab 4
m = 1360.0
Crr = 0.02
Cd  = 0.5
A   = 2.0
rw  = 0.28
theta = np.deg2rad(10)     # try 0 first to sanity-check
vw = 2.0
eta_g = 0.95
gear = 1.65                 # <-- reduction ratio (motor:wheel)
rho = 1.2
g = 9.81
dt = 0.001

# *** GUI Code: ***
