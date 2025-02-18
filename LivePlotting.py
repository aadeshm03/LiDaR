import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# Read the data
df = pd.read_csv("C:/Users/User/OneDrive - UBC/Desktop/BMEG 457/sensor_data.csv", index_col=0)
df = df.reset_index()

# Create figure and axis
fig, ax = plt.subplots(figsize=(10, 6))
line, = ax.plot([], [], 'b-', linewidth=2)

# Add horizontal dotted lines for error boundaries
ax.axhline(y=-20, color='r', linestyle=':', label='Upper Bound (-20)')
ax.axhline(y=-35, color='r', linestyle=':', label='Lower Bound (-35)')

# Set the plot limits
ax.set_xlim(df['Time'].min() - 1, df['Time'].max() + 1)
ax.set_ylim(df['ReorientedLidar'].min() - 2, df['ReorientedLidar'].max() + 2)

# Add labels and title
ax.set_xlabel('Time')
ax.set_ylabel('Reoriented Lidar')
ax.set_title('Reoriented Lidar Measurements Over Time')
ax.grid(True)
ax.legend()

# Initialize function for the animation
def init():
    line.set_data([], [])
    return line,

# Animation function
def animate(frame):
    x_data = df['Time'][:frame+1]
    y_data = df['ReorientedLidar'][:frame+1]
    line.set_data(x_data, y_data)
    return line,

# Create the animation
anim = FuncAnimation(fig, animate, 
                    init_func=init,
                    frames=len(df), 
                    interval=200,
                    blit=True,
                    repeat=True)

# Show the plot
plt.show()