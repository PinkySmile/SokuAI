#!/bin/env python3
import sys
import tqdm
import torch
import time
from tensordict import TensorDict
from tensordict.nn import TensorDictModule
from torch import nn, optim
from torchrl.envs import CatTensors, TransformedEnv, UnsqueezeTransform

from Soku.Env import MoveBasedSokuEnv
from Soku.characters import REMILIA
from Soku.weathers import CLEAR

if len(sys.argv) < 3:
    print("Usage:", sys.argv[0], "<game_path> [<SWRSToys.ini>]")

client = sys.argv[1]
action_list = ["6a", "nothing", "forward", "backward"]
env = MoveBasedSokuEnv(client, action_list, start_port=10800)
in_keys = [
    ('params', 'left', 'character'),
    ('params', 'right', 'character'),
    ('left', 'opponentRelativePos.x'),
    ('left', 'opponentRelativePos.y'),
    ('left', 'score'),
    ('left', 'hp'),
    ('left', 'action'),
    ('left', 'pose'),
    ('left', 'poseFrame'),
    ('left', 'frameCount'),
    ('left', 'comboDamage'),
    ('left', 'time_idle'),
    ('right', 'opponentRelativePos.x'),
    ('right', 'opponentRelativePos.y'),
    ('right', 'score'),
    ('right', 'hp'),
    ('right', 'action'),
    ('right', 'pose'),
    ('right', 'poseFrame'),
    ('right', 'frameCount'),
    ('right', 'comboDamage'),
    ('right', 'time_idle')
]
env = TransformedEnv(
    env,
    # ``Unsqueeze`` the observations that we will concatenate
    UnsqueezeTransform(
        dim=-1,
        in_keys=in_keys,
        in_keys_inv=[],
    ),
)
env.append_transform(CatTensors(in_keys=in_keys, dim=-1, out_key="observation", del_keys=False))
torch.manual_seed(time.time())
net = nn.Sequential(
    nn.Linear(len(in_keys), 128),
    nn.ReLU(),
    nn.Linear(128, 512),
    nn.ReLU(),
    nn.Linear(512, 512),
    nn.ReLU(),
    nn.Linear(512, len(action_list)),
)
policy = TensorDictModule(
    net,
    in_keys=["observation"],
    out_keys=["action"],
)
optimizer = optim.Adam(policy.parameters(), lr=2e-3)
batch_size = 1
pbar = tqdm.tqdm(range(20_000 // batch_size))
scheduler = optim.lr_scheduler.CosineAnnealingLR(optimizer, 20_000)
logs = {"return": [], "last_reward": []}
init_params = TensorDict({
    "params": TensorDict({
        "left":  TensorDict({ "character": REMILIA, "deck": [0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4], "palette": 0, "name": "Test1" }),
        "right": TensorDict({ "character": REMILIA, "deck": [0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4], "palette": 1, "name": "Test2" }),
        "stage": 0,
        "music": 0,
        "vs_com": False
    }, [])
}, []).expand(batch_size).contiguous()

env.set_game_volume(0, 0)
env.set_game_speed(100000)
env.set_display_mode(False)
for _ in pbar:
    init_td = env.reset(init_params)
    env.set_health(100, 100)
    env.set_weather(CLEAR, 0, True)
    env.set_restricted_moves([200, 201])
    rollout = env.rollout(600, policy, tensordict=init_td, auto_reset=False)
    traj_return = rollout["next", "reward"].mean()
    (-traj_return).backward()
    gn = nn.utils.clip_grad_norm_(net.parameters(), 1.0)
    last = rollout[..., -1]['next', 'reward'].mean(dtype=torch.float32)
    optimizer.step()
    optimizer.zero_grad()
    pbar.set_description(f"reward: {traj_return: 4.4f}, last reward: {last: 4.4f}, gradient norm: {gn: 4.4}")
    logs["return"].append(traj_return.item())
    logs["last_reward"].append(last.item())
    scheduler.step()
