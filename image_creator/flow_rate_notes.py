# GPT says:

# Certainly! Calculating the time to open a solenoid valve to reach a desired pressure in a system involves considering the flow rate through the valve, the system volume, and the desired pressure. Here's a simplified Python script that outlines the basic calculation:


import math

# Constants
valve_coefficient = 0.8  # Valve flow coefficient (depends on the specific valve)
system_volume = 10.0  # System volume in cubic meters
desired_pressure = 80  # Desired pressure in PSI
initial_pressure = 14.6959488  # Initial pressure in the system
ambient_pressure = 14.6959488  # Ambient pressure in PSI (standard atmospheric pressure)

# Calculate the flow rate through the valve using the flow coefficient
flow_rate = valve_coefficient * math.sqrt((2 * (desired_pressure - ambient_pressure)) / 1000)  # Flow rate in cubic meters per second

# Calculate the time to reach the desired pressure
time_to_reach_pressure = (system_volume * (desired_pressure - initial_pressure)) / flow_rate  # Time in seconds

print("Time to open the solenoid valve: {:.2f} seconds".format(time_to_reach_pressure))


# In this script:
# - We use the flow coefficient of the solenoid valve to calculate the flow rate through the valve.
# - Then, we calculate the time required to reach the desired pressure in the system based on the system volume, initial pressure, and the calculated flow rate.

# Please note that this is a simplified example and does not take into account factors such as valve dynamics, pressure drop, temperature effects, and other real-world complexities that may affect the actual time required to reach the desired pressure. For precise engineering calculations, it's important to consider these additional factors and consult relevant standards and guidelines.

# If you have specific details about the solenoid valve, system, and desired pressure, I can tailor the script




# My idea:
# 1. go to zero & max tank, calculate all values...
# 2. air up for 3 seconds, 1 or all bags or a mix idk
# 3. get new values relative to time...
# 4. repeat 2 and 3 so we can get a curve????
# can probably use that to calculate air up time? I am unsure. Maybe need to do step 2 twice to get 2 points to generate a graph of some sort idk bruh.
# Will likely require we control the compressor so we can pause it during the calculations.