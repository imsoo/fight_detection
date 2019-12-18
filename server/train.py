import tensorflow as tf
import numpy as np
from tensorflow import keras
from tensorflow.keras import layers
config = tf.ConfigProto()
config.gpu_options.allow_growth = True
sess = tf.Session(config=config)

# Set parameter
n_input = 24  # num input parameters per timestep
n_steps = 32
n_hidden = 34 # Hidden layer num of features
n_classes = 4 
batch_size = 1024
lambda_loss_amount = 0.0015
learning_rate = 0.0025
decay_rate = 0.02
training_epochs = 1000

###
def load_X(X_path):
    file = open(X_path, 'r')
    X_ = np.array(
        [elem for elem in [
            row.split(',') for row in file
        ]], 
        dtype=np.float32
    )
    file.close()
    blocks = int(len(X_) / n_steps)
    X_ = np.array(np.split(X_,blocks))
    return X_ 

def load_y(y_path):
    file = open(y_path, 'r')
    y_ = np.array(
        [elem for elem in [
            row.replace('  ', ' ').strip().split(' ') for row in file
        ]], 
        dtype=np.int32
    )
    file.close()
    # for 0-based indexing 
    return y_ - 1


DATASET_PATH = "data/HAR_pose_activities/database/"

X_train_path = DATASET_PATH + "X_pose36.txt"
X_test_path = DATASET_PATH + "X_custom.txt"

y_train_path = DATASET_PATH + "Y_pose36.txt"
y_test_path = DATASET_PATH + "Y_custom.txt"


X_train = load_X(X_train_path)
X_test = load_X(X_test_path)
#print X_test

y_train = load_y(y_train_path)
y_test = load_y(y_test_path)
###

model = tf.keras.Sequential([
   # relu activation
   layers.Dense(n_hidden, activation='relu', 
       kernel_initializer='random_normal', 
       bias_initializer='random_normal',
       batch_input_shape=(batch_size, n_steps, n_input)
   ),
   
   # cuDNN
   layers.CuDNNLSTM(n_hidden, return_sequences=True,  unit_forget_bias=1.0),
   layers.CuDNNLSTM(n_hidden,  unit_forget_bias=1.0),
   
   # layers.LSTM(n_hidden, return_sequences=True,  unit_forget_bias=1.0),
   # layers.LSTM(n_hidden,  unit_forget_bias=1.0),

   layers.Dense(n_classes, kernel_initializer='random_normal', 
       bias_initializer='random_normal',
       kernel_regularizer=tf.keras.regularizers.l2(lambda_loss_amount),
       bias_regularizer=tf.keras.regularizers.l2(lambda_loss_amount),
       activation='softmax'
   )
])

model.compile(
   optimizer=tf.keras.optimizers.Adam(lr=learning_rate, decay=decay_rate),
   metrics=['accuracy'],
   loss='categorical_crossentropy'
)

y_train_one_hot = keras.utils.to_categorical(y_train, 4)
y_test_one_hot = keras.utils.to_categorical(y_test, 4)

train_size = X_train.shape[0] - X_train.shape[0] % batch_size
test_size = X_test.shape[0] - X_test.shape[0] % batch_size

history = model.fit(
   X_train[:train_size,:,:], 
   y_train_one_hot[:train_size,:], 
   epochs=training_epochs,
   batch_size=batch_size,
   validation_data=(X_test[:test_size,:,:], y_test_one_hot[:test_size,:])
)

from tensorflow.keras.models import load_model
model.save('action.h5')
