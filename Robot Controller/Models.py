from torch.distributions import Normal
import torch
import torch.nn as nn
from torch_geometric.nn import HeteroConv, GCNConv, GraphConv, GATConv,  global_mean_pool
from torch_geometric.data import HeteroData

from typing import Any, Dict, Optional
import numpy as np

from ray.rllib.core.columns import Columns
from ray.rllib.core.rl_module.apis.value_function_api import ValueFunctionAPI
from ray.rllib.core.rl_module.torch import TorchRLModule
from ray.rllib.utils.annotations import override
from ray.rllib.utils.framework import try_import_torch
from ray.rllib.utils.typing import TensorType
from ray.rllib.models.torch.torch_distributions import TorchDiagGaussian
from ray.rllib.algorithms.ppo.torch.ppo_torch_rl_module import PPOTorchRLModule
from ray.rllib.core.rl_module.rl_module import RLModule

torch, nn = try_import_torch()

default_edge_index_dict = {
    ('joint', 'bone', 'joint'): torch.tensor([
            [0, 1, 2, 3, 4, 5,   7, 8,  9,   11, 12, 13,   15, 16, 17,   19, 20, 21],       
            [1, 2, 3, 4, 5, 6,   8, 9, 10,   12, 13, 14,   16, 17, 18,   20, 21, 22]    
        ], dtype=torch.long), 
    ('joint', 'end_effector', 'joint'): torch.tensor([
            [6,  6,  6,  6,     10, 10, 10,     14, 14,     18,],       
            [10, 14, 18, 22,    14, 18, 22,     18, 22,     22,]    
        ], dtype=torch.long), 
    ('joint', 'symmetry', 'joint'): torch.tensor([
            [7,  8,  9,  10,   15, 16, 17, 18],       
            [11, 12, 13, 14,   19, 20, 21, 22]    
        ], dtype=torch.long),
    ('joint', 'limb', 'joint'): torch.tensor([
            [0, 0, 0, 0, 0, 0,   7,  7,  7,   11, 11, 11,   15, 15, 15,   19, 19, 19],       
            [1, 2, 3, 4, 5, 6,   8,  9, 10,   12, 13, 14,   16, 17, 18,   20, 21, 22]    
        ], dtype=torch.long),

    ('imu', 'linked', 'joint'): torch.tensor([
        [0],       
        [6]    
    ], dtype=torch.long),
    ('joint', 'linked', 'imu'): torch.tensor([
        [6],       
        [0]    
    ], dtype=torch.long)
}

def preprocess_edge_index_dict(edge_index_dict):
    def fully_connect_subgraphs(edges):
        # Identify unique source nodes (subgraphs)
        unique_sources = edges[0].unique()

        # Detect subgraphs
        subgraphs = {}

        for source in unique_sources:
            # Get the target nodes associated with the current source
            targets = edges[1][edges[0] == source]
            subgraphs[source.item()] = targets.tolist()

        # Now, make each subgraph fully connected
        fully_connected_edges = []

        for source, targets in subgraphs.items():
            # Add the original edges first
            for target in targets:
                fully_connected_edges.append([source, target])

            # Now, fully connect the target nodes with each other
            for i in range(len(targets)):
                for j in range(i + 1, len(targets)):
                    # Add the edge between target[i] and target[j]
                    fully_connected_edges.append([targets[i], targets[j]])

        # Convert the fully connected edges to a tensor
        fully_connected_edges = torch.tensor(fully_connected_edges, dtype=torch.long).t()
        return fully_connected_edges
    
    # Ensure bidirectional connections for other relations
    def make_bidirectional(edges):
        return torch.cat([edges, torch.flip(edges, [0])], dim=1)

    # Connect the 'end_effector' and 'limb' nodes by fully connecting them
    edge_index_dict[('joint', 'end_effector', 'joint')] = make_bidirectional(fully_connect_subgraphs(edge_index_dict[('joint', 'end_effector', 'joint')]))
    edge_index_dict[('joint', 'limb', 'joint')] = make_bidirectional(fully_connect_subgraphs(edge_index_dict[('joint', 'limb', 'joint')]))

    edge_index_dict[('joint', 'bone', 'joint')] = make_bidirectional(edge_index_dict[('joint', 'bone', 'joint')])
    edge_index_dict[('joint', 'symmetry', 'joint')] = make_bidirectional(edge_index_dict[('joint', 'symmetry', 'joint')])

    return edge_index_dict

def get_batched_edge_index_dict(edge_index_dict, batch_size, node_offsets_dict):
    """
    Returns a batched edge_index_dict with offsets applied to each graph in the batch.

    Args:
    - edge_index_dict (dict): Dictionary containing the edge indices for each relationship type.
    - batch_size (int): Number of graphs in the batch.
    - node_offsets_dict (dict): Dictionary containing the offsets for each node type.

    Returns:
    - batched_edge_index_dict (dict): Updated edge index dictionary with node indices offset for each graph in the batch.
    """
    batched_edge_index_dict = {}

    # Iterate over each relation type in the original edge_index_dict
    for (node_type_a, edge_type, node_type_b), edge_index in edge_index_dict.items():
        # Prepare a list to hold all the edge indices for this edge type
        batched_edge_indices = []

        # Get the offsets for both node types
        offset_a = node_offsets_dict[node_type_a]
        offset_b = node_offsets_dict[node_type_b]

        # Apply offset for each batch graph
        for i in range(batch_size):
            # Apply the offsets to the edge index tensor
            edge_index_offset = edge_index.clone()  # Make a copy to avoid modifying the original tensor
            edge_index_offset[0] += offset_a * i  # Apply offset to the first node type (node_type_a)
            edge_index_offset[1] += offset_b * i  # Apply offset to the second node type (node_type_b)

            # Add the offset edge indices to the batched_edge_indices list
            batched_edge_indices.append(edge_index_offset)

        # Concatenate all the edge indices for the batch into one tensor
        batched_edge_index_dict[(node_type_a, edge_type, node_type_b)] = torch.cat(batched_edge_indices, dim=-1)

    return batched_edge_index_dict



class CustomRLModule(TorchRLModule, ValueFunctionAPI):
    @override(TorchRLModule)
    def setup(self):
        self.num_joints = self.model_config.get("num_joints", 23)
        self.num_imus = self.model_config.get("num_imus", 1)
        joint_channels = self.model_config.get("joint_channels", 6) + self.num_joints
        self.joint_channels = joint_channels
        imu_channels = self.model_config.get("imu_channels", 7)
        self.imu_channels = imu_channels
        hidden_channels = self.model_config.get("hidden_channels", 32)
        self.hidden_channels = hidden_channels
        activation_function = self.model_config.get("activation_function", nn.LeakyReLU())
        graph_conv_layers = self.model_config.get("graph_conv_layers", 6)
        self.edge_index_dict = preprocess_edge_index_dict(self.model_config.get("edge_index_dict", default_edge_index_dict))
        GATHeads = self.model_config.get("gat_heads", 2)
        self.lstm_layers = self.model_config.get("lstm_layers", 2)

        #Feature Extractors
        self.joint_feature_extractor =  nn.Sequential(
            nn.Linear(joint_channels, (joint_channels+hidden_channels)//2),
            activation_function,
            nn.Linear((joint_channels+hidden_channels)//2, hidden_channels),
            activation_function,
        )
        self.imu_feature_extractor = nn.Sequential(
            nn.Linear(imu_channels, (imu_channels+hidden_channels)//2),
            activation_function,
            nn.Linear((imu_channels+hidden_channels)//2, hidden_channels),
            activation_function,
        )
        
        #Body
        self.convolutions = nn.ModuleList([
            HeteroConv({
                ('joint', 'end_effector', 'joint'): GATConv(hidden_channels, hidden_channels, heads = GATHeads, concat=False),
                ('joint', 'limb', 'joint'): GATConv(hidden_channels, hidden_channels, heads = GATHeads, concat=False),
                
                ('joint', 'bone', 'joint'): GATConv(hidden_channels, hidden_channels, heads = 2, concat=False),
                ('joint', 'symmetry', 'joint'): GATConv(hidden_channels, hidden_channels, heads = 2, concat=False),
                
                ('joint', 'linked', 'imu'): GATConv(hidden_channels, hidden_channels, heads = 1, concat=False, add_self_loops=False),
                ('imu', 'linked', 'joint'): GATConv(hidden_channels, hidden_channels, heads = 1, concat=False, add_self_loops=False)
            }, aggr='sum') for _ in range(graph_conv_layers)
        ])
        self.joint_lstm = nn.LSTM(input_size=self.hidden_channels, hidden_size=self.hidden_channels, batch_first=True, num_layers=self.lstm_layers)

        #Output Heads
        self.actions_per_joint=3
        self.joints_policy_mean = nn.Sequential(
            nn.Linear(hidden_channels, (hidden_channels+self.actions_per_joint)//2),
            activation_function,
            nn.Linear((hidden_channels+self.actions_per_joint)//2, self.actions_per_joint)
        )
        self.log_std = nn.Parameter(torch.zeros(1, 1, self.actions_per_joint))
        
        self.value_head = nn.Sequential(
            nn.Linear(hidden_channels, hidden_channels),
            activation_function,
            nn.Linear(hidden_channels, hidden_channels//2),
            activation_function,
            nn.Linear(hidden_channels//2, 1, bias=False),
        )

    @override(TorchRLModule)
    def get_initial_state(self) -> Any:
        return {
            "h": np.zeros(shape=(self.lstm_layers, self.num_joints, self.hidden_channels,), dtype=np.double),
            "c": np.zeros(shape=(self.lstm_layers, self.num_joints, self.hidden_channels,), dtype=np.double),
        }

    @override(RLModule)
    def _forward(self, batch: Dict[str, Any], **kwargs) -> Dict[str, Any]:
        obs = batch[Columns.OBS]
        state_in = batch[Columns.STATE_IN]
        joints = obs['joints'] # B, N, M | NOW: B, T, N, M
        imus = obs['imus'] # B, N, M | NOW: B, T, N, M
        B, T, N, M = joints.shape

        joints_flat = joints.reshape(B*T*self.num_joints, self.joint_channels)  # (B * T * N, M)
        imus_flat = imus.reshape(B*T*self.num_imus, self.imu_channels)  # (B * T * N, M)
        joints_feat_flat = self.joint_feature_extractor(joints_flat)
        imus_feat_flat = self.imu_feature_extractor(imus_flat)
        
        x_dict = {
            'joint': joints_feat_flat,
            'imu': imus_feat_flat
        }
        if B*T > 1:
            edge_index_dict = get_batched_edge_index_dict(self.edge_index_dict, B*T, {'joint': self.num_joints, 'imu': self.num_imus})
        else:
            edge_index_dict = self.edge_index_dict
        for conv in self.convolutions:
            x_dict = conv(x_dict, edge_index_dict)
        joints_feat_flat = x_dict['joint'] 

        joint_embeddings_out = joints_feat_flat.reshape(B, T, N, self.hidden_channels)
        joint_embeddings = joint_embeddings_out.permute(0, 2, 1, 3) #change to B, N, T, F
        joint_embeddings = joint_embeddings.reshape(B*N, T, self.hidden_channels)

        joint_state_hidden = state_in['h']
        joint_state_hidden = joint_state_hidden.permute(1, 0, 2, 3)
        joint_state_hidden = joint_state_hidden.reshape(self.lstm_layers, B * self.num_joints, self.hidden_channels)
        joint_state_cell = state_in['c']
        joint_state_cell = joint_state_cell.permute(1, 0, 2, 3)
        joint_state_cell = joint_state_cell.reshape(self.lstm_layers, B * self.num_joints, self.hidden_channels)

        joint_embeddings_out, (joint_state_hidden_out, joint_state_cell_out) = self.joint_lstm(
            joint_embeddings, 
            (joint_state_hidden, joint_state_cell)
        )
        joint_embeddings_out = joint_embeddings_out.reshape(B, N, T, self.hidden_channels)
        joint_embeddings_out = joint_embeddings_out.permute(0, 2, 1, 3)

        joint_state_hidden_out = joint_state_hidden_out.view(self.lstm_layers, B, N, self.hidden_channels)
        joint_state_hidden_out = joint_state_hidden_out.permute(1, 0, 2, 3)
        joint_state_cell_out = joint_state_cell_out.reshape(self.lstm_layers, B, N, self.hidden_channels)
        joint_state_cell_out = joint_state_cell_out.permute(1, 0, 2, 3)

        joints_embeddings_flat = joint_embeddings_out.reshape(B*T*N, self.hidden_channels)

        joints_out_mean_flat = self.joints_policy_mean(joints_embeddings_flat)
        joints_out_mean = joints_out_mean_flat.reshape(B, T, N, self.actions_per_joint)
        joints_out_log_std = self.log_std.expand_as(joints_out_mean)
        joints_out = torch.cat([joints_out_mean, joints_out_log_std], dim=-1)
        lstm_hidden_state = {
            "h": joint_state_hidden_out,
            "c": joint_state_cell_out, 
        }

        return {
            Columns.EMBEDDINGS: joint_embeddings_out,
            Columns.STATE_OUT: lstm_hidden_state,
            Columns.ACTION_DIST_INPUTS: joints_out,
        }

    @override(RLModule)
    def _forward_train(self, batch: Dict[str, Any], **kwargs) -> Dict[str, Any]:
        outs = self._forward(batch)
        return outs

    # We implement this RLModule as a ValueFunctionAPI RLModule, so it can be used
    # by value-based methods like PPO or IMPALA.
    @override(ValueFunctionAPI)
    def compute_values(
        self, batch: Dict[str, Any], embeddings: Optional[Any] = None
    ) -> TensorType: # type: ignore
        
        if embeddings is None:
            embeddings = batch[Columns.EMBEDDINGS]

        B = embeddings.shape[0]
        T = embeddings.shape[1]
        N = embeddings.shape[2]
        M = embeddings.shape[3]

        flattened_embeddings = embeddings.reshape(B*T*N, M)
        batch = torch.cat([torch.full((N,), i, dtype=torch.long) for i in range(B*T)])
        pooled_embeddings = global_mean_pool(flattened_embeddings, batch)
        values = self.value_head(pooled_embeddings)
        reshaped_values = torch.squeeze(values.view(B, T, 1), dim=2)
        return reshaped_values
                                                        
joints = ['spine_02', 'spine_03', 'spine_04', 'spine_05', 'neck_01', 'neck_02', 'head',
         'clavicle_l', 'upperarm_l', 'lowerarm_l', 'hand_l',
         'clavicle_r', 'upperarm_r', 'lowerarm_r', 'hand_r',
         'thigh_l', 'calf_l', 'foot_l', 'ball_l',
         'thigh_r', 'calf_r', 'foot_r', 'ball_r',
         ]
imus = ['head'] #, 'hand_l', 'hand_r', 'foot_l', 'foot_r'

x_dict = {
    'joint': torch.zeros(len(joints), 16),  # nodes, features
    'imu': torch.zeros(len(imus), 16)
}

# -------------------------------------------------------------------------------------------------------------------------------------------------
# -------------------------------------------------------------------------------------------------------------------------------------------------
# -------------------------------------------------------------------------------------------------------------------------------------------------
# -------------------------------------------------------------------------------------------------------------------------------------------------

# 1. Base Encoder Class
class Encoder(nn.Module):
    def __init__(self, input_dim, model_dim, num_layers=1, activation_fn=nn.GELU):
        super().__init__()
        layers = []
        for _ in range(num_layers):
            layers.append(nn.Linear(input_dim, model_dim, dtype=torch.float))
            layers.append(activation_fn())
        self.encoder = nn.Sequential(*layers)

    def forward(self, data):
        return self.encoder(data)

# 3. Base Decoder Class
class Decoder(nn.Module):
    def __init__(self, model_dim, output_dim, num_layers=1, activation_fn=nn.ReLU):
        super().__init__()
        layers = [nn.Linear(model_dim, model_dim, dtype=torch.float), activation_fn()]
        for _ in range(num_layers - 1):
            layers.append(nn.Linear(model_dim, model_dim, dtype=torch.float))
            layers.append(activation_fn())
        layers.append(nn.Linear(model_dim, output_dim, dtype=torch.float))
        self.decoder = nn.Sequential(*layers)

    def forward(self, encoded_tokens):
        return self.decoder(encoded_tokens)

# Pass-through Encoder (does nothing)
class PassThroughEncoder(nn.Module):
    def __init__(self):
        super().__init__()

    def forward(self, data):
        return data  # Just return the input unchanged

# Pass-through Decoder (does nothing)
class PassThroughDecoder(nn.Module):
    def __init__(self):
        super().__init__()

    def forward(self, encoded_tokens):
        return encoded_tokens  # Just return the input unchanged


class Transformer(nn.Module):
    def __init__(self, encoders, decoders, model_dim, num_heads, num_layers, num_instances, activation_fn=nn.GELU):
        super().__init__()
        self.encoders = nn.ModuleDict(encoders)
        self.decoders = nn.ModuleDict(decoders)
        self.model_dim = model_dim

        self.instance_embedding = nn.Embedding(num_instances, model_dim, dtype=torch.float)
        self.type_embedding = nn.Embedding(len(encoders), model_dim, dtype=torch.float) # +1 for state token type, doesnt have encoder

        transformer_layer = nn.TransformerEncoderLayer(model_dim, num_heads, activation=activation_fn(), batch_first=True, dtype=torch.float)
        self.transformer = nn.TransformerEncoder(transformer_layer, num_layers)

        self.state_str = "state"
    
    def forward(self, input_data):
        modality_token_indices = {}
        tokens = []
        token_count = 0

        for modality in input_data.keys():
            #print("modal: ", modality)
            data, ids, type_id = input_data[modality]
            #print("b: ", data.type())
            
            encoded_features = self.encoders[modality](data)
            instance_embed = self.instance_embedding(ids)
            type_embed = self.type_embedding(torch.full_like(ids, type_id))
            # print("a: ", type(instance_embed), type(encoded_features), type(type_embed))
            modality_tokens = encoded_features + instance_embed + type_embed
            tokens.append(modality_tokens)
            
            modal_token_count = modality_tokens.shape[1]
            new_token_count = token_count+modal_token_count
            modality_token_indices[modality] = slice(token_count, new_token_count)  # Store indices for decoding
            token_count = new_token_count
            #print("t len:", modality_token_indices)
        
        #print(tokens[0].shape, tokens[1].shape, tokens[2].shape, tokens[3].shape, tokens[4].shape)
        tokens = torch.cat(tokens, dim=1)
        encoded_tokens = self.transformer(tokens)
        #print(encoded_tokens.shape)

        outputs = {}
        for modality in modality_token_indices.keys():
            if modality in self.decoders.keys():
                #modality_slice = modality_token_indices.get(modality, slice(0, 0))
                decoded_tokens = encoded_tokens[:, modality_token_indices[modality], :]
                #print('ttype: ', type(decoded_tokens), modality, decoded_tokens.shape)
                outs = self.decoders[modality](decoded_tokens)
                #print(outs.shape)
                outputs[modality] = outs

        return outputs

    def get_default_tokens(self, num_tokens):
        return np.zeros(shape=(num_tokens, self.model_dim,), dtype=np.float64)

class TransformerRobotModule(TorchRLModule, ValueFunctionAPI):
    @override(TorchRLModule)
    def setup(self):
        self.transformer_channels = self.model_config.get("transformer_channels", 32)
        self.transformer_layers = self.model_config.get("transformer_layers", 2)
        self.transformer_heads = self.model_config.get("transformer_heads", 1)
        self.transformer_activation = self.model_config.get("joint_activation", nn.GELU)
        self.num_states = self.model_config.get("num_states", 1)

        self.num_joints = self.model_config.get("num_joints", 23)
        self.joint_channels = self.model_config.get("joint_channels", 6)

        self.num_imus = self.model_config.get("num_imus", 1)
        self.imu_channels = self.model_config.get("imu_channels", 7)
        #self.reward_embeddings = self.model_config.get("")

        state_encoder = PassThroughEncoder()
        reward_encoder = PassThroughEncoder()
        clock_encoder = Encoder(
            input_dim=1,
            model_dim=self.transformer_channels,
            num_layers=1
        )

        servo_encoder = Encoder(
            input_dim=self.joint_channels, 
            model_dim=self.transformer_channels, 
            num_layers=1
        )
        imu_encoder = Encoder(
            input_dim=self.imu_channels, 
            model_dim=self.transformer_channels, 
            num_layers=1
        )
        
        state_decoder = PassThroughDecoder()
        reward_decoder = PassThroughDecoder()
        
        servo_decoder = Decoder(
            model_dim=self.transformer_channels, 
            output_dim=3,
            num_layers=self.transformer_layers, 
            activation_fn=self.transformer_activation
        )
        

        encoders = {
            "state": state_encoder,
            "reward": reward_encoder,
            "clock": clock_encoder,

            "servo": servo_encoder,
            "imu": imu_encoder
        }
        self.num_instances = self.num_imus + self.num_joints + self.num_states + 2 #+1 for reward, +1 for clock

        self.instances = {
            "state": torch.arange(0, self.num_states, dtype=torch.int32),
            "reward": torch.tensor([self.num_states], dtype=torch.int32),
            "clock": torch.tensor([self.num_states + 1], dtype=torch.int32),
            "servo": torch.arange(self.num_states + 2, self.num_states + 2 + self.num_joints, dtype=torch.int32),
            "imu": torch.arange(self.num_states + 2 + self.num_joints, self.num_instances, dtype=torch.int32),
        }
        self.types = {
            "state": 0,
            "reward": 1,
            "clock": 2,

            "servo": 3,
            "imu": 4
        }
        
        decoders = {
            "state": state_decoder,
            "reward": reward_decoder,

            "servo": servo_decoder
        }

        
        self.transformer = Transformer(
            encoders=encoders,
            decoders=decoders,
            model_dim=self.transformer_channels,
            num_heads=self.transformer_heads,
            num_layers=self.transformer_layers,
            num_instances=self.num_instances,
            activation_fn=self.transformer_activation,
        )

        self.value_decoder = Decoder(
            model_dim=self.transformer_channels,
            output_dim=1,
            num_layers=1,
            activation_fn=self.transformer_activation
        )

        servo_actions = 3
        self.log_std = nn.Parameter(torch.zeros(1, 1, servo_actions))

    @override(TorchRLModule)
    def get_initial_state(self) -> Any:
        return {'states': self.transformer.get_default_tokens(self.num_states)}

    @override(RLModule)
    def _forward(self, batch: Dict[str, Any], **kwargs) -> Dict[str, Any]:
        obs = batch[Columns.OBS]
        state_in = batch[Columns.STATE_IN] # B, N, M

        # joints = obs['joints'] # B, T, N, M
        # imus = obs['imus'] # B, T, N, M
        # clock = obs['clock'] # B, T, N, M

        # Move T to the first dimension
        joints = obs['joints'].permute(1, 0, 2, 3)  # T, B, N, M
        imus = obs['imus'].permute(1, 0, 2, 3)  # T, B, N, M
        clock = obs['clocks'].permute(1, 0, 2, 3)  # T, B, N, M

        state_tokens = state_in['states']

        embeddings = None
        action_means = None

        # Iterate through time steps
        for t in range(joints.shape[0]):  # Loop over T
            joints_t = joints[t]  # Shape: B, N, M
            imus_t = imus[t]  # Shape: B, N, M
            clock_t = clock[t]  # Shape: B, N, M
            B = joints_t.shape[0]
            reward_t = torch.from_numpy(self.transformer.get_default_tokens(B).astype(np.float32)).unsqueeze(1) # gives shape B, M... change to B, 1, M, where N=1
            #print("reward type: ", reward_t.type()) 
            #print("pre T:", reward_pre_t.shape, reward_t.shape)


            input_data = {
                "state": (state_tokens, self.instances["state"], self.types["state"]),
                "reward": (reward_t, self.instances["reward"], self.types["reward"]),
                "clock": (clock_t, self.instances["clock"], self.types["clock"]),

                "servo": (joints_t, self.instances["servo"], self.types["servo"]),
                "imu": (imus_t, self.instances["imu"], self.types["imu"]),
            }

            output_data = self.transformer(input_data)
            state_tokens = output_data["state"]
            reward_embeddings = output_data["reward"].unsqueeze(0)
            action_dist = output_data["servo"].unsqueeze(0)
            if embeddings is None:
                embeddings = reward_embeddings
            else:
                embeddings = torch.cat([embeddings, reward_embeddings], dim=0)
            if action_means is None:
                action_means = action_dist
            else:
                action_means = torch.cat([action_means, action_dist], dim=0)

            
            #print(reward_embeddings.shape, action_means.shape)

        #print('alog: ', type(action_means))
        action_log_std = self.log_std.expand_as(action_means)
        #print('fin1')
        action_dist_inputs = torch.cat([action_means, action_log_std], dim=-1)
        #reshape to B, T, N, M
        embeddings = embeddings.permute(1, 0, 2, 3)
        action_dist_inputs = action_dist_inputs.permute(1, 0, 2, 3)

        #print('finished')
        #print(embeddings.shape, state_tokens.shape, action_dist_inputs.shape)
        state_out = {"states": state_tokens}

        dict_out = {
            Columns.EMBEDDINGS: embeddings,
            Columns.STATE_OUT: state_out,
            Columns.ACTION_DIST_INPUTS: action_dist_inputs,
        }

        return dict_out



    @override(RLModule)
    def _forward_train(self, batch: Dict[str, Any], **kwargs) -> Dict[str, Any]:
        #print("training")
        #print("batch keys:", batch.keys())
        outs = self._forward(batch)
        return outs
    

    @override(ValueFunctionAPI)
    def compute_values(
        self, batch: Dict[str, Any], embeddings: Optional[Any] = None
    ) -> TensorType: # type: ignore
        
        if embeddings is None:
            embeddings = batch[Columns.EMBEDDINGS]

        B = embeddings.shape[0]
        T = embeddings.shape[1]
        N = embeddings.shape[2]
        M = embeddings.shape[3]

        flattened_embeddings = embeddings.reshape(B*T*N, M)
        values = self.value_decoder(flattened_embeddings)
        reshaped_values = torch.squeeze(values.view(B, T, 1), dim=2)
        return reshaped_values