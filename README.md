# AI Powered Snake Game on Arduino <br>UCR CS120b Final Project

This repository contains the final project for the UCR CS120b course. The project implements a Snake game using an Arduino for the game logic and a Python script for running a reinforcement learning model to provide auto-play functionality.
- **Demo Video**: [Watch the demo video](https://youtu.be/qagf-ed95OY)

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Setup](#hardware-setup)
- [Software Setup](#software-setup)
- [Usage](#usage)
- [Training The Model](#training-the-model)

## Overview

The project combines an Arduino-based Snake game with a reinforcement learning model. The game logic is handled by the Arduino, while a Python script communicates with the Arduino to run a PPO model for auto-play.

## Features

- **Manual Play**: Play the Snake game using physical controls connected to the Arduino.
- **Auto-Play**: Enable auto-play using a reinforcement learning model.
- **Game State Display**: Display the game state on an 8x8 LED matrix.
- **Score Tracking**: Keep track of the player's score and high scores using non-volatile memory.
- **Game Sounds**: Play game sounds on a passive buzzer.

## Hardware Setup

- **Arduino Uno**
- **8x8 LED Matrix**
- **LCD Display**
- **Joystick Module**
- **Passive Buzzer**
- **Wires and Breadboard**

### Connections

- Connect the LED matrix to the Arduino using SPI (digital pins 10, 11, and 13).
- Connect the joystick module to the analog pins of the Arduino (Analog pins 0 - 2).
- Connect the buzzer to digital pin 9.
- Connect the LCD display to digital pins 2 - 7.

## Software Setup

### Arduino

The Arduino code (`main.c`) is responsible for the game logic and managing the hardware. It communicates with the Python script via serial communication.

### Python

The Python script (`inference.py`) runs a reinforcement learning model to provide auto-play functionality. It uses the Stable Baselines3 library to load and run the model.

## Usage

### Manual Play

1. Power on the Arduino.
2. Use the joystick to control the snake and play the game.

### Auto-Play

1. Run the `inference.py` script:
    ```sh
    python src/auto_mode/inference.py
    ```
2. The script will use the trained model to control the snake.

## Training the Model

The model is trained using the **Proximal Policy Optimisation (PPO)** strategy. The OpenAI-Gym environment, defined in `environment.py`, is a faithful simulation of the game running on the Arduino. For training the model, the **stable-baselines3** implementation of PPO was utilized. Detailed information and the training process can be found in `model.ipynb`.

### Key Points:
- **Algorithm**: Proximal Policy Optimization (PPO)
- **Environment**: OpenAI-Gym simulation (`environment.py`)
- **Library**: stable-baselines3
- **Notebook**: `model.ipynb` for the training process

