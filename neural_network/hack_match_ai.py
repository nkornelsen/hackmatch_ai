from tf_agents.environments import tf_py_environment
from tf_agents.trajectories import time_step
from tf_agents.agents.dqn import dqn_agent
from tf_agents.utils import common
from tf_agents.policies import random_tf_policy
from tf_agents import networks
from tf_agents.replay_buffers import tf_uniform_replay_buffer
from tf_agents.drivers import dynamic_episode_driver
from tf_agents.utils import common
from tf_agents.metrics import tf_metrics

import numpy as np
import tensorflow as tf
tf.config.run_functions_eagerly(True)
from tensorflow import debugging
import environment
import os
import struct
import argparse


parser = argparse.ArgumentParser()
parser.add_argument("--continue", action="store_true")
load_ckpt = vars(parser.parse_args())["continue"]
print(f"Load from checkpoint: {load_ckpt}")

### PARAMETERS ###
replay_buffer_max_length = 5000
random_run_episodes = 1
training_episodes = 1
num_episodes = 0
log_interval = 1
checkpoint_interval = 20
change_temperature_interval = 20
training_batch_size = 128
evaluate_interval = 10

hm_environment = environment.HackMatchEnvironment()
# remember to connect the environment ! 
hm_environment._connect_socket()
tf_env = tf_py_environment.TFPyEnvironment(hm_environment)
print("TimeStep Specs:", hm_environment.time_step_spec())
print("Action Specs:", hm_environment.action_spec())

input_img = tf.keras.Input(shape=(240, 145, 3), dtype=tf.dtypes.float16)
input_prev_action = tf.keras.Input(shape=(5,), dtype=tf.dtypes.float32)


def create_conv_layer(num_channels):
    return tf.keras.layers.Conv2D(
        num_channels,
        (3, 3),
        (1, 1),
        ("same"),
        activation=tf.keras.activations.relu,
        kernel_initializer=tf.keras.initializers.VarianceScaling()
    )

# series of conv layers
# Input convolution layer
conv_input = tf.keras.layers.Conv2D(
    10,
    (6, 6),
    (5, 6),
    "valid",
    activation=tf.keras.activations.relu,
    input_shape=(240, 145, 3),
    kernel_initializer=tf.keras.initializers.VarianceScaling()
)

conv_1 = create_conv_layer(40)

conv_2 = create_conv_layer(40)

flatten_1 = tf.keras.layers.Flatten()

concat_1 = tf.keras.layers.Concatenate()

dense_1 = tf.keras.layers.Dense(
    40,
    activation=tf.keras.activations.relu,
    kernel_initializer=tf.keras.initializers.VarianceScaling()
)

dense_2 = tf.keras.layers.Dense(
    80,
    activation=tf.keras.activations.relu,
    kernel_initializer=tf.keras.initializers.VarianceScaling()
)

dense_3 = tf.keras.layers.Dense(
    160,
    activation=tf.keras.activations.relu,
    kernel_initializer=tf.keras.initializers.VarianceScaling()
)

dense_output = tf.keras.layers.Dense(
    5,
    activation=None,
    kernel_initializer=tf.keras.initializers.RandomUniform(minval=-0.08, maxval=0.08)
)

# model = tf.keras.models.Model(inputs=[input_img, input_prev_action], outputs=dense_output)
# q_network = networks.Sequential([model])
q_network = networks.Sequential([
    networks.NestMap({"board": networks.Sequential([conv_input, conv_1, conv_2, tf.keras.layers.Flatten()]),
                      "prev_action": tf.keras.layers.Flatten()}),
    networks.NestFlatten(),
    tf.keras.layers.Concatenate(),
    dense_1,
    dense_2,
    dense_3,
    dense_output
])


optimizer = tf.keras.optimizers.Adam(learning_rate=0.0005)
train_step_counter = tf.Variable(0, dtype=tf.int64)

temperature_counter = 0

@tf.function
def get_epsilon():
    global temperature_counter
    return 0.7*np.exp(-0.15*temperature_counter)

agent = dqn_agent.DqnAgent(
    tf_env.time_step_spec(),
    tf_env.action_spec(),
    q_network=q_network,
    optimizer=optimizer,
    td_errors_loss_fn=common.element_wise_squared_loss,
    train_step_counter=train_step_counter,
    epsilon_greedy=get_epsilon)

agent.initialize()

def compute_average_return(environment, policy, num_episodes=5):
    total_return = 0.0
    for _ in range(num_episodes):
        time_step = environment.reset()
        episode_return = 0.0
        print("Next episode")

        while not time_step.is_last():
            action_step = policy.action(time_step)
            time_step = environment.step(action_step.action)
            episode_return += time_step.reward
        total_return += episode_return

    avg_return = total_return / num_episodes
    return avg_return.numpy()[0]


replay_buffer = tf_uniform_replay_buffer.TFUniformReplayBuffer(
    agent.collect_data_spec,
    batch_size=1,
    max_length=replay_buffer_max_length
)

# driver = dynamic_episode_driver.DynamicEpisodeDriver(
#     tf_env,
#     random_policy,
#     [rb_observer],
#     num_episodes=4
# )

agent.train = common.function(agent.train)

agent.train_step_counter.assign(0)

# avg_return = compute_average_return(tf_env, agent.policy, num_episodes=2)

# print(f"Average return before training: {avg_return}")

# returns = [avg_return]

returns = []
# collect_driver = dynamic_episode_driver.DynamicEpisodeDriver(
#     tf_env,
#     agent.collect_policy,
#     [rb_observer],
#     num_episodes=2
# )
# random_policy = random_py_policy.RandomPyPolicy(hm_environment.time_step_spec(), hm_environment.action_spec())
random_policy = random_tf_policy.RandomTFPolicy(tf_env.time_step_spec(), tf_env.action_spec())

# collect_driver = py_driver.PyDriver(
#     hm_environment,
#     py_tf_eager_policy.PyTFEagerPolicy(
#         agent.collect_policy,
#         use_tf_function = True
#     ),
#     [replay_buffer.add_batch],
#     max_episodes=5
# )

summary_dir = os.path.join(".", "metrics")
summary_writer = tf.summary.create_file_writer(summary_dir, flush_millis=1000)
summary_writer.set_as_default()

episode_counter = tf.Variable(0, dtype=tf.int64)

train_metrics = [
    tf_metrics.NumberOfEpisodes(),
    tf_metrics.AverageReturnMetric(),
    tf_metrics.AverageEpisodeLengthMetric()
]
    
collect_driver = dynamic_episode_driver.DynamicEpisodeDriver(
    tf_env,
    agent.collect_policy,
    [replay_buffer.add_batch] + train_metrics,
    num_episodes=training_episodes
)


# start with random data
random_driver = dynamic_episode_driver.DynamicEpisodeDriver(
    tf_env,
    random_policy,
    [replay_buffer.add_batch],
    num_episodes=random_run_episodes
)

checkpoint_dir = os.path.join('.', 'checkpoint')
train_checkpointer = common.Checkpointer(
    ckpt_dir=checkpoint_dir,
    agent=agent,
    policy=agent.policy,
    global_step=train_step_counter
)

dataset = replay_buffer.as_dataset(
    sample_batch_size=training_batch_size, 
    num_steps=2,
    single_deterministic_pass=False)

iterator = iter(dataset)

time_step = tf_env.reset()
if not load_ckpt:
    print("Running random driver")
    time_step, _ = random_driver.run(time_step)
    print("Completed random driver run")
else:
    print("Loading checkpoint")
    train_checkpointer.initialize_or_restore()
    print("Filling replay buffer")
    time_step, _ = collect_driver.run(time_step)
    print("Replay buffer initialized, starting training")

while True:
    if num_episodes != 0:
        if episode_counter.value() >= num_episodes:
            break
    print(f"Episode {episode_counter.value()}")
    loss = []
    i = 0
    
    # pause the game
    send_action = 7
    binary_value = struct.pack("!i", send_action)
    hm_environment._sock.send(binary_value)

    
    for train_metric in train_metrics:
        train_metric.tf_summaries(train_step=episode_counter, step_metrics=train_metrics[1:])

    # print("Training...")
    # for experience, unused_info in dataset:
    #     if i % 50 == 0:
    #         print(f"{i} samples trained")
    #     i += 1
    #     training = agent.train(experience)
    #     loss.append(float(training.loss.numpy()))
    experience, unused_info = next(iterator)
    training = agent.train(experience)
    loss = float(training.loss.numpy())

    replay_buffer.clear()
    step = agent.train_step_counter.numpy()
    
    if step % log_interval == 0:
        print(f"step = {step}, loss = {loss}")

    if episode_counter.value() % checkpoint_interval == 0:
        print("Saving checkpoint!")
        train_checkpointer.save(train_step_counter)
    
    if episode_counter.value() % change_temperature_interval == 0:
        temperature_counter += 1

    # unpause the game
    send_action = 8
    binary_value = struct.pack("!i", send_action)
    hm_environment._sock.send(binary_value)

    if episode_counter.value() % evaluate_interval == 0:
        avg_return = compute_average_return(num_episodes=1, environment=tf_env, policy=agent.policy)
        print(f"step = {step}, avg_return = {avg_return}")
    
    time_step, _ = collect_driver.run(time_step)
    episode_counter.assign_add(1)
