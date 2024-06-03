import numpy as np
from gymnasium import spaces
import gymnasium as gym
import matplotlib.pyplot as plt

class SnakeGame(gym.Env):

    metadate = {"render.modes": ["console", "rgb_array"]}
    n_actions = 4

    # Directions
    UP = 0
    RIGHT = 1
    DOWN = 2
    LEFT = 3

    # Possible outputs
    STRAIGHT = 1
    TURN_LEFT = 0
    TURN_RIGHT = 2

    # Value interpretations for grid array
    EMPTY = 0
    SNAKE_BODY = 1
    SNAKE_HEAD = 2
    FOOD = 4

    # Rewards
    REWARD_HIT = -20
    REWAED_STEP_TOWARDS_FOOD = 1
    REWARD_PER_FOOD = 50
    MAX_STEPS_AFTER_FOOD = 200

    def __init__(self, grid_size=8):
        super().__init__()
        self.stepnum = 0
        self.last_food_step = 0

        # Build grid
        self.grid_size = grid_size
        self.grid = np.zeros((self.grid_size, self.grid_size), dtype=np.uint8) + self.EMPTY
        self.snake_coordinates = [(3, 3)]
        self.snake_direction = self.UP
        for coord in self.snake_coordinates:
          self.grid[coord] = self.SNAKE_BODY
        self.grid[self.snake_coordinates[-1]] = self.SNAKE_HEAD

        # Add food
        self.food = (np.random.randint(grid_size), np.random.randint(grid_size))
        while (self.grid[self.food] != 0):
            self.food = (np.random.randint(grid_size), np.random.randint(grid_size))
        self.grid[self.food] = self.FOOD
        self.dist_to_food = self.grid_distance(self.snake_coordinates[-1], self.food)

        self.init_grid = self.grid.copy()
        self.init_snake_coordinates = self.snake_coordinates.copy()

        self.actions_space = spaces.Discrete(self.n_actions)

        self.observation_space = spaces.Dict(
            spaces={
                "direction": spaces.Box(low=0, high=3, shape=(1,), dtype=np.int32),
                "grid": spaces.Box(low=0, high=3, shape=(self.grid_size, self.grid_size), dtype=np.uint8)
            }
        )

    def get_state(self):
        scaled_grid = self.grid / 3.0  # Scale grid values to [0, 1]
        scaled_direction = np.array([self.snake_direction / 3.0], dtype=np.float32)  # Scale direction to [0, 1]
        return {
            "direction": scaled_direction,
            "grid": scaled_grid
        }


    def step(self, action):
        self.stepnum += 1

        # Update snake_direction based on action
        if action == self.TURN_RIGHT:
            self.snake_direction = (self.snake_direction + 1) % 4
            pass
        elif action == self.TURN_LEFT:
            self.snake_direction = (self.snake_direction - 1) % 4

        # Get new head coordinates based on the current direction
        head_x, head_y = self.snake_coordinates[-1]
        if self.snake_direction == self.UP:
            head_x = (head_x - 1) % self.grid_size
        elif self.snake_direction == self.DOWN:
            head_x = (head_x + 1) % self.grid_size
        elif self.snake_direction == self.LEFT:
            head_y = (head_y - 1) % self.grid_size
        elif self.snake_direction == self.RIGHT:
            head_y = (head_y + 1) % self.grid_size
        new_head = (head_x, head_y)

        reward = 0
        done = False
        # Check for collisions with the snake's own body
        if new_head in self.snake_coordinates:
            reward = self.REWARD_HIT
            done = True
            return self.get_state(), reward, done, {}

        # Check if the snake has found food
        if new_head == self.food:
            reward = self.REWARD_PER_FOOD
            self.snake_coordinates.append(new_head)
            self.grid[self.snake_coordinates[-2]] = self.SNAKE_BODY
            self.grid[new_head] = self.SNAKE_HEAD

            # Check if snake takes up the entige screen
            if len(self.snake_coordinates) == (self.grid_size * self.grid_size):
                done = True
            else:
                # Generate new food position
                self.food = (np.random.randint(self.grid_size), np.random.randint(self.grid_size))
                while self.grid[self.food] != self.EMPTY:
                    self.food = (np.random.randint(self.grid_size), np.random.randint(self.grid_size))
                self.grid[self.food] = self.FOOD
                self.last_food_step = self.stepnum
        else:
            reward = self.REWAED_STEP_TOWARDS_FOOD
            self.snake_coordinates.append(new_head)
            self.grid[new_head] = self.SNAKE_HEAD
            tail = self.snake_coordinates.pop(0)
            self.grid[tail] = self.EMPTY

            if len(self.snake_coordinates) > 1:
              self.grid[self.snake_coordinates[-2]] = self.SNAKE_BODY

        # Check if maximum steps after finding food is reached
        if self.stepnum - self.last_food_step >= self.MAX_STEPS_AFTER_FOOD:
            done = True
        else:
            done = False

        return self.get_state(), reward, done, {}


    def reset(self, seed=None):
        self.stepnum = 0
        self.last_food_step = 0
        self.grid = self.init_grid.copy()
        self.snake_coordinates = self.init_snake_coordinates.copy()
        
        self.dist_to_food = self.grid_distance(self.snake_coordinates[-1], self.food)

        if seed is not None:
            np.random.seed(seed)

        state = self.get_state()
        info = {}

        return state, info

    def grid_distance(self, pos1, pos2):
        return np.linalg.norm(np.array(pos1, dtype=np.float32)-np.array(pos2, dtype=np.float32))

    def snake_plot(self, plot_inline=False):
        snake_ind = (self.grid == self.SNAKE_BODY)
        food_ind = (self.grid == self.FOOD)
        head_ind = (self.grid == self.SNAKE_HEAD)

        # Create color array for plot, default white color
        color_array = np.zeros((self.grid_size, self.grid_size, 3), dtype=np.uint8) + 255
        color_array[snake_ind, :] = np.array([0, 255, 0]) # Blue snake
        color_array[food_ind, :] = np.array([255, 0, 0])  # Green food
        color_array[head_ind, :] = np.array([0, 0, 255])  # Red snakehead
        return color_array

    def render(self, mode="rgb_array"):
        if mode == "console":
            print(self.grid)
        elif mode == "rgb_array":
            return self.snake_plot()