import numpy as np
import socket
import struct

from tf_agents.environments import py_environment
from tf_agents.environments import utils
from tf_agents.specs import array_spec
from tf_agents.trajectories import time_step

import tensorflow as tf

frames_received = 0
def get_next_state(sock):
    global frames_received
    if sock is None:
        raise Exception("Socket not initialized")
    # receive the score
    
    score_data = sock.recv(4, socket.MSG_WAITALL)
    score = struct.unpack("!i", score_data)[0]
    # print(score)
    if score == -1:
        print("Game over")
        return (None, None)
    img_array = np.empty(240*145*3, dtype=np.uint8)
    bytes_left = 240*145*3
    while bytes_left > 0:
        arr_offset = 240*145*3 - bytes_left
        sock.recv_into(img_array[arr_offset:], min(bytes_left, 16384), socket.MSG_WAITALL)
        bytes_left -= min(bytes_left, 16384)
    img_array = np.reshape(img_array, (240, 145, 3))[...,::-1]
    frames_received += 1
    if frames_received % 300 == 0:
        print(f"Total frames: {frames_received}")
    
    return (np.uint32(score), img_array.astype(np.float16))

class HackMatchEnvironment(py_environment.PyEnvironment):
    def __init__(self):
        # nothing, z, x, left, right, hold left, hold right
        self._action_spec = array_spec.BoundedArraySpec(shape=(), dtype=np.uint8, minimum=0, maximum=4, name="action")
        self._observation_spec = {
            "board": array_spec.BoundedArraySpec(shape=(240, 145, 3), dtype=np.float16, minimum=0, maximum=256.0, name="observation_board"),
            "prev_action": array_spec.BoundedArraySpec(shape=(5,), dtype=np.float32, minimum=0, maximum=1.0, name="observation_prev_action")
        }
        self._current_score = 0
        self._episode_ended = False
        self._sock = None
    
    def _connect_socket(self):
        self._sock = socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM)
        self._sock.connect((2, 0x5f0))

    def action_spec(self):
        return self._action_spec
    
    def observation_spec(self):
        return self._observation_spec
    
    def _reset(self):
        self._current_score = 0
        self._episode_ended = False
        prev_action = np.zeros((5,), dtype=np.float32)
        prev_action[0] = 1.0
        (score, img_array) = get_next_state(self._sock)
        
        return time_step.restart(observation={
            "board": img_array,
            "prev_action": prev_action
        })

    def _step(self, action):
        if self._episode_ended:
            return self._reset()
        send_action = int(action)
        binary_value = struct.pack("!i", send_action)
        self._sock.send(binary_value)
        prev_action = np.zeros((5,), dtype=np.float32)
        prev_action[send_action] = 1.0
        (score, img_array) = get_next_state(self._sock)
        reward = 0.0
        if score != self._current_score:
            if score is None:
                self._episode_ended = True
                return time_step.termination(observation={
                    "board": np.zeros((240, 145, 3), dtype=np.float16),
                    "prev_action": prev_action}, 
                    reward=np.float32(0.0))
            reward = score - self._current_score
            if reward != 0.0:
                print(f"Reward: {reward}")
            self._current_score = score

        observation = {
            "board": img_array,
            "prev_action": prev_action
        }
        return time_step.transition(observation=observation, reward=np.float32(reward+0.1), discount=0.90)
    
if __name__ == "__main__":
    environment = HackMatchEnvironment()
    environment._connect_socket()
    utils.validate_py_environment(environment)