import tensorflow as tf
import serial
import time
import numpy as np
from QModel import QModel, create_q_model

"""
model_path = "/Users/finnferchau/Desktop/Hochschule/Sem-4/ES/Lab/CustomLab/Custom_Lab/UCR-CS120b-Final-Project/models/snake_dqn_model.tflite"

# Load the TensorFlow Lite model
interpreter = tf.lite.Interpreter(model_path=model_path)
interpreter.allocate_tensors()

# Get input and output tensors
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()
"""

# Load the model
model_path = '/Users/finnferchau/Desktop/Hochschule/Sem-4/ES/Lab/CustomLab/Custom_Lab/UCR-CS120b-Final-Project/models/snake_dqn_model.keras'  # Replace with the correct path

# Create the QModel with the correct parameters
model = create_q_model(grid_size=8, num_actions=3)

# Now you can try loading the weights
try:
  model.load_weights(model_path)
  print("Model loaded successfully.")
except Exception as e:
  print(f"Error loading model: {e}")

# Prepare input data
def prepare_input(grid, direction):
    grid = np.array(grid).reshape((1, 8, 8, 1))  # Reshape grid to match model input shape
    direction = np.array([direction]).reshape((1, 1))  # Ensure direction is in the correct shape
    return [grid, direction]

# Example grid and direction
example_grid = np.zeros(64)  # Replace with your actual grid data
example_direction = 2  # Replace with your actual direction data

# Prepare the input
inputs = prepare_input(example_grid, example_direction)

# Make predictions
predictions = model.predict(inputs)
print(predictions)

# Set up serial communication
ser = serial.Serial('/dev/tty.usbmodem21101', 9600)
time.sleep(2)  # Wait for the serial connection to initialize

def get_inference(data):
    # Check if the length of the data is correct
    if len(data) != 65:
        print("Received data has incorrect number of bytes.")
        return None

    # Convert byte data to list of integers
    input_data = [int(byte) for byte in data]
    print(f"Int data: {input_data}")
    
    # Extract grid and direction from input_data
    grid = np.array(input_data[:64])
    grid[grid == 3] = 4 # Error inside game env
    grid = grid / 3.0   # Scale grid
    grid = grid.reshape((1, 8, 8, 1))  # First 64 elements
    direction = np.array([input_data[64]])
    direction = direction / 3.0 # Scale direction
    direction = direction.reshape((1, 1))  # Last element

    # Prepare the inputs
    inputs = [grid, direction]
    
    # Run the model
    output_data = model.predict(inputs)
    return output_data[0]




while True:
        if ser.in_waiting > 0:
            # Read the incoming data from Arduino
            data = ser.readline().strip()
            print(f"Received data: {data}")
            
            # Get the inference from the model
            result = get_inference(data)
            
            # Extract the action with the highest value
            predicted_action = np.argmax(result)
            
            # Send the result back to Arduino
            ser.write(f"{result}\n".encode('utf-8'))
            print(f"Sent result: {predicted_action}")
