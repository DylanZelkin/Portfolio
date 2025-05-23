import ray
from ray.tune.registry import register_env
from ray.rllib.algorithms.ppo import PPOConfig
from ray.rllib.algorithms import PPOConfig

from Models import CustomRLModule, TransformerRobotModule
from ray.rllib.core.rl_module import RLModuleSpec

from UnrealEnvironment import UnrealEnvironment
from UnrealEnvironment import UnrealManager
import gymnasium as gym
from gymnasium.spaces import flatten_space, flatten

from MetricVisualizer import GraphWindow
import time
import os
import gc
import sys
import numpy as np
from gymnasium import spaces

#### NOTE NOTE NOTE NOTE NOTE THERE ARE MODIFCATIONS TO: (USING RAY 2.39.0) (search for "ADDED" and "MODIFIED") -> for transformer usage, otherwise use normal .venv without mods
#   venv\Lib\site-packages\ray\rllib\connectors\common\add_states_from_episodes_to_batch.py
#   venv\Lib\site-packages\ray\rllib\connectors\common\batch_individual_items.py
#   venv\Lib\site-packages\ray\rllib\algorithms\ppo\torch\ppo_torch_learner.py

os.environ["RAY_DEDUP_LOGS"] = "0"
print("code start...")

class FlattenNestedDictWrapper(gym.ObservationWrapper):
    def __init__(self, env):
        super().__init__(env)
        # Convert the potentially nested observation space into a flat space.
        #a = flatten_space(env.observation_space)
        self.observation_space = flatten_space(env.observation_space)

    def observation(self, observation):
        # Use gymnasium.flatten to convert the nested observation into a flat array.
        a = flatten(self.env.observation_space, observation)
        #a = np.array([[0, 1, 2], [1, 2]])
        return a

def create_my_env(config):
    return FlattenNestedDictWrapper(UnrealEnvironment(config))

envName = "UnrealEnvironment"
register_env(envName, create_my_env)

#envName = "UnrealTransformerEnv"

ray.init(ignore_reinit_error=True,
         #runtime_env={"env_vars": {"USE_LIBUV": "0"}}
         )

manager = UnrealManager()

# Create a PPOConfig object
config = PPOConfig()
config.framework('torch')
config.api_stack(
    enable_rl_module_and_learner=True,
    enable_env_runner_and_connector_v2=True
)
#Set your desired configuration values
config.environment( 
    env=envName, 
    env_config={
        "envID": 0, 
        "action": 0,
        "transformation_rotation_weight": 1.0,
        "action_diff_scaler": 0.0,
        "action_less_energy_weight": 0.0,
        "max_steps": 128
    }
)
config.learners(
    num_learners=0,
    num_gpus_per_learner=0,
)
config.resources(num_cpus_for_main_process=0)
config.env_runners(
    num_env_runners=15,
    batch_mode='complete_episodes',
    num_gpus_per_env_runner=0,
    num_cpus_per_env_runner=0,
    #rollout_fragment_length=100
) 
config.training(
    train_batch_size=128*30,
    num_epochs = 30,
    #lr = 1e-5,
    lr=[
        [0, 1e-5],
        [100000, 1e-5]
    ],
    minibatch_size=64*6,
    vf_clip_param=100,
    clip_param=1,
    vf_loss_coeff=1,
    #kl_coeff = 0.05,
    #kl_target = 0.1,
    #grad_clip=5,
    #lambda_ = 0.33
)

config.rl_module(
        model_config={
            'use_lstm': True,
            'lstm_cell_size': 128,
            'max_seq_len': 64,
            "lstm_use_prev_action": True,
            "lstm_use_prev_reward": True,

            'fcnet_activation': 'elu',
            'fcnet_hiddens': [128,128],

            'use_attention': True,
            'attention_num_heads': 2,
            'attention_dim': 64,
            "attention_use_n_prev_actions": 64,
            "attention_use_n_prev_rewards": 64,
            #'keep_per_episode_custom_metrics': True
        },
)

# config.rl_module(   rl_module_spec=RLModuleSpec(module_class=CustomRLModule),
#                     model_config={
#                         'use_lstm': True,
#                         'max_seq_len': 64,
#                         "hidden_channels": 64,
#                         "graph_conv_layers": 4,
#                         "lstm_layers": 1,
#                         "gat_heads": 2
#                     },               
#                  )

# config.rl_module(   rl_module_spec=RLModuleSpec(module_class=TransformerRobotModule),
#                     model_config={
#                         'use_lstm': True,
#                         'max_seq_len': 64,

#                         'num_states': 2,
#                         'transformer_channels': 128,
#                         'transformer_layers': 2,
#                         'transformer_heads': 4,
#                     },               
#                  )

print("starting training...")

trainer = config.build()
visualizer = GraphWindow.start()
start_time = time.time()

abs_path = os.path.abspath("Agents/Standing/Checkpoint")
total_iterations = 10000
iterations_per_save = 100
reload_from_iteration = 4800
if reload_from_iteration <= 0:
    best_reward = -1000000
    iterations = range(total_iterations+1)
else:
    checkpoint_path = f"{abs_path}_{reload_from_iteration}"
    if not os.path.isdir(checkpoint_path):
        raise FileNotFoundError(f"Checkpoint directory {checkpoint_path} not found.")
    print(f"Loading checkpoint from {checkpoint_path}...")
    trainer.restore(checkpoint_path)
    visualizer.load_metrics(filepath = checkpoint_path)
    best_reward = visualizer.get_best_reward()
    iterations = range(reload_from_iteration+1, total_iterations+1)



evaluating = False
evaluations = 3
if evaluating:
    for eval in range(evaluations):
        trainer.evaluate()
        exit(0)



for i in iterations: 
    print("--------------------")
    print(f"starting iteration {i+1}")
    result = trainer.train()
    print(f"cleaning up iteration.....")
    gc.collect()
    
    env_runners = result['env_runners']
    episode_reward_mean = env_runners['episode_return_mean']
    episode_len_mean = env_runners['episode_len_mean']

    policy = result['learners']['default_policy']
    entropy = policy['entropy']
    policy_loss = policy['policy_loss']
    value_loss = policy['vf_loss']
    mean_kl_loss = policy['mean_kl_loss']
    vf_explained_var = policy['vf_explained_var']

    print(f"cleaning up environment.......")
    fps = manager.getFPS()

    visualizer.update_graph1(episode_reward_mean, entropy)
    visualizer.update_graph2(policy_loss, value_loss)
    visualizer.update_graph3(mean_kl_loss, vf_explained_var)
    visualizer.update_graph4(fps, 0)

    if episode_reward_mean > best_reward:
        print(f"New best reward! saving.....")
        path = abs_path + "_Best"
        best_reward = episode_reward_mean
        checkpoint_path = trainer.save(path)
        visualizer.save_graph(filepath = path + "/")
        
    if i % iterations_per_save == 0:
        print(f"scheduled saving...")
        path = abs_path+f"_{i}"
        checkpoint_path = trainer.save(path)
        visualizer.save_graph(filepath = path + "/")
    else:
        print(f"saving most recent")
        path = abs_path+f"_Newest"
        checkpoint_path = trainer.save(path)
        visualizer.save_graph(filepath = path + "/")

    current_time = time.time()
    print(f"Completed!! | Running for {(current_time-start_time)/60} minutes...")

    

ray.shutdown()