import tensorflow as tf
from tensorflow.keras.layers import Dense, Flatten, Concatenate, Input
from tensorflow.keras.models import Model

class QModel(tf.keras.Model):
    def __init__(self, grid_size, num_actions, **kwargs):
        super(QModel, self).__init__(**kwargs)
        self.grid_size = grid_size
        self.num_actions = num_actions
        self.flatten = Flatten()
        self.dense1 = Dense(128, activation='relu')
        self.dense2 = Dense(128, activation='relu')
        self.output_layer = Dense(num_actions, activation='linear')

    def call(self, inputs):
        grid, direction = inputs
        grid_flat = self.flatten(grid)
        direction = tf.cast(direction, tf.float32)  # Ensure direction is float for concatenation
        concat = Concatenate()([grid_flat, direction])
        x = self.dense1(concat)
        x = self.dense2(x)
        output = self.output_layer(x)
        return output

    def get_config(self):
        config = super(QModel, self).get_config()
        config.update({
            'grid_size': self.grid_size,
            'num_actions': self.num_actions
        })
        return config

    @classmethod
    def from_config(cls, config):
        return cls(**config)

# Ensure the model can be saved and loaded with the custom objects
def create_q_model(grid_size, num_actions):
    grid_input = Input(shape=(grid_size, grid_size, 1), name='grid')
    direction_input = Input(shape=(1,), name='direction')

    model = QModel(grid_size, num_actions)
    outputs = model([grid_input, direction_input])

    q_model = Model(inputs=[grid_input, direction_input], outputs=outputs)
    q_model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate=0.001),
                    loss='mse')
    return q_model