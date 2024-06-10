import torch
import serial
import time
import numpy as np
from stable_baselines3 import PPO
import matplotlib.pyplot as plt

# Load the model
model_path = '/Users/finnferchau/Desktop/Hochschule/Sem-4/ES/Lab/CustomLab/Custom_Lab/UCR-CS120b-Final-Project/models/model.zip'

# Load the PyTorch model
try:
    model = PPO.load(model_path)
    print("Model loaded successfully.")
except Exception as e:
    print(f"Error loading model: {e}")
    quit()

def snake_plot(grid, plot_inline=False):
    # Value interpretations for grid array
    EMPTY = 0
    SNAKE_BODY = 1
    SNAKE_HEAD = 2
    FOOD = 3 

    snake_ind = (grid == SNAKE_BODY)
    food_ind = (grid == FOOD)
    head_ind = (grid == SNAKE_HEAD)

    # Create color array for plot, default white color
    color_array = np.zeros((8, 8, 3), dtype=np.uint8) + 255
    color_array[snake_ind, :] = np.array([0, 255, 0]) # Blue snake
    color_array[food_ind, :] = np.array([255, 0, 0])  # Green food
    color_array[head_ind, :] = np.array([0, 0, 255])  # Red snakehead

    # Plot the rendered image
    plt.imshow(color_array)
    plt.gca().invert_yaxis()  # Invert the y-axis
    plt.gca().xaxis.set_ticks_position('bottom')  # Set x-axis ticks to the bottom
    plt.gca().yaxis.set_ticks_position('left')    # Set y-axis ticks to the left
    plt.show(block=False)
    plt.pause(0.1)  # Pause to allow the plot to update
    plt.clf()  # Clear the figure to update the plot in the next iteration


def prepare_input(grid, direction, position):
    # Prepare the grid
    grid = grid.reshape((8, 8))  # Reshape grid to match observation space

    # Plot the snake
    snake_plot(grid)

    grid = grid.reshape((1, 8, 8))

    # Convert to tensors
    grid_tensor = torch.tensor(grid, dtype=torch.uint8)
    direction_tensor = torch.tensor([direction], dtype=torch.int32).unsqueeze(0)
    position_tensor = torch.tensor(position, dtype=torch.int32).unsqueeze(0)

    # Return a dictionary matching the observation space
    return {
        'grid': grid_tensor,
        'direction': direction_tensor,
        'position': position_tensor
    }

def get_inference(data):
    # Convert byte data to list of integers
    input_data = [int(byte) for byte in data]
    
    # Extract grid and direction from input_data
    grid = np.array(input_data[:64])
    direction = np.array([input_data[64]])
    position = input_data[65:67]

    print(f"Direction: {direction}")
    print(f"Position: {position}")

    # Prepare the inputs
    state = prepare_input(grid, direction, position)
    
    # Run the model
    with torch.no_grad():
        action, _states = model.predict(state)
        print(action)
    return action

# Set up serial communication
try:
    ser = serial.Serial('/dev/tty.usbmodem21101', 9600)
    ser.reset_input_buffer()
    print(f"Connected to Serial")
except Exception as e:
    print("Could not connect to serial:")
    print(e)
    quit()
time.sleep(2)  # Wait for the serial connection to initialize
ser.readline().strip()

while True:
    if ser.in_waiting > 0:
        # Read the incoming data from Arduino
        data = ser.readline().strip()
        print(f"Received data: {data}")

        # Check if the length of the data is correct
        if len(data) != 67:
            print(f"Received data has incorrect number of bytes. Length: {len(data)}")
            continue
        
        # Get the inference from the model
        predicted_action = get_inference(data)[0]
        print(f"Action: {predicted_action}")
        # Extract the action with the highest value
        # predicted_action = np.argmax(result)
        
        # Send the result back to Arduino
        ser.write(f"{predicted_action}\n".encode("utf-8"))
        print(f"Sent result: {predicted_action}")
