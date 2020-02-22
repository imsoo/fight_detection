import tensorflow as tf
import numpy as np
import zmq
import io
import time
from tensorflow import keras

from tensorflow.keras.backend import set_session
config = tf.ConfigProto()
config.gpu_options.allow_growth = True
sess = tf.Session(config=config)
set_session(sess)
sess.run(tf.global_variables_initializer())

'''
## TF 2.0
from tensorflow.compat.v1.keras.backend import set_session
tf.compat.v1.disable_eager_execution()
config = tf.compat.v1.ConfigProto()  
config.gpu_options.allow_growth = True
sess = tf.compat.v1.Session(config=config)
set_session(sess)
sess.run(tf.compat.v1.global_variables_initializer())
'''

model = keras.models.load_model("weights/action.h5")

n_input = 24  # num input parameters per timestep
n_steps = 32
n_hidden = 34 # Hidden layer num of features
n_classes = 4 
batch_size = 1024

def load_X(msg):
  buf = io.StringIO(msg)
  X_ = np.array(
      [elem for elem in [
      row.split(',') for row in buf
      ]], 
      dtype=np.float32
      )
  blocks = int(len(X_) / 32)
  X_ = np.array(np.split(X_,blocks))
  return X_ 


# load
input_ = np.zeros((batch_size, n_steps, n_input), dtype=np.float32)
print("model loaded ...")

context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("ipc://action")

while True:
  msg = socket.recv()
  msg = msg.decode("utf-8")
  recv_ = load_X(msg)

  for i in range(len(recv_)):
    input_[i] = recv_[i]
  startTime = time.time()
  pred = model.predict_classes(input_, batch_size = batch_size)
  
  endTime = time.time() - startTime
  print("time : ", endTime)
  pred_str = ""

  for i in range(len(recv_)):
    pred_str += str(pred[i])
  print("result : ", pred_str)
  socket.send_string(pred_str)
