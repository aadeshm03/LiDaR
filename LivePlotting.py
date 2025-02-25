import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# Read the data
df = pd.read_csv("C:/Users/User/OneDrive - UBC/Desktop/BMEG 457/sensor_data.csv")
dfTracker = pd.read_csv("C:/Users/User/OneDrive - UBC/Desktop/BMEG 457/Tracker_data.csv")

# Reset index for consistency
df = df.reset_index(drop=True)
dfTracker = dfTracker.reset_index(drop=True)

# Create figure and axis
fig, ax = plt.subplots(figsize=(10, 6))
line, = ax.plot([], [], 'b-', linewidth=2, label='Reoriented Lidar')
lineT, = ax.plot([], [], 'g-', linewidth=2, label='Tracker Distance')

# Add horizontal dotted lines for error boundaries
ax.axhline(y=-20, color='r', linestyle=':', label='Upper Bound (-20)')
ax.axhline(y=-35, color='r', linestyle=':', label='Lower Bound (-35)')

# Set plot limits
ax.set_xlim(df["Time"].min() - 1, df["Time"].max() + 1)
ax.set_ylim(min(df["ReorientedLidar"].min(), dfTracker["Distance"].min()) - 2,
            max(df["ReorientedLidar"].max(), dfTracker["Distance"].max()) + 2)

# Add labels and title
ax.set_xlabel("Time")
ax.set_ylabel("Value")
ax.set_title("Reoriented Lidar and Tracker Measurements Over Time")
ax.grid(True)
ax.legend()

# Initialize function for animation
def init():
    line.set_data([], [])
    lineT.set_data([], [])
    return line, lineT

# Animation function
def animate(frame):
    # Update Reoriented Lidar data
    x_data = df["Time"][:frame + 1]
    y_data = df["ReorientedLidar"][:frame + 1]
    
    # Update Tracker Distance data
    X_data = dfTracker["Time"][:frame + 1]
    Y_data = dfTracker["Distance"][:frame + 1]
    
    line.set_data(x_data, y_data)
    lineT.set_data(X_data, Y_data)
    return line, lineT

# Create the animation
anim = FuncAnimation(fig, animate, 
                     init_func=init,
                     frames=min(len(df), len(dfTracker)), 
                     interval=200,
                     blit=False,  # Set blit to False for compatibility
                     repeat=True)

# Show the plot
plt.show()
