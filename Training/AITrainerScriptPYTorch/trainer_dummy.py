#!/bin/env python3
import sys
import tqdm
from tensordict import TensorDict
from tensordict.nn import TensorDictModule
from torch import nn, optim
from torchrl.envs import CatTensors, TransformedEnv

from Soku.Env import SokuEnv
from Soku.characters import REMILIA
from Soku.weathers import CLEAR

if len(sys.argv) < 3:
    print("Usage:", sys.argv[0], "<game_path> [<SWRSToys.ini>]")

client = sys.argv[1]
port = 10800
while True:
    try:
        env = SokuEnv(client, port)
        break
    except OSError:
        port += 1
cat_transform = CatTensors(in_keys=[
    ('params', 'left', 'character'),
    ('params', 'left', 'deck'),
    ('params', 'right', 'character'),
    ('params', 'right', 'deck'),
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
], dim=-1, out_key="observation", del_keys=False)
env = TransformedEnv(env, cat_transform)
net = nn.Sequential(
    nn.LazyLinear(64),
    nn.Tanh(),
    nn.LazyLinear(64),
    nn.Tanh(),
    nn.LazyLinear(64),
    nn.Tanh(),
    nn.LazyLinear(5),
)
policy = TensorDictModule(
    net,
    in_keys=["observation"],
    out_keys=["action"],
)
optimizer = optim.Adam(policy.parameters(), lr=2e-3)
batch_size = 32
pbar = tqdm.tqdm(range(20_000 // batch_size))
scheduler = optim.lr_scheduler.CosineAnnealingLR(optimizer, 20_000)
logs = {"return": [], "last_reward": []}
init_params = TensorDict({
    "params": {
        "left": { "character": REMILIA, "deck": [0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4], "palette": 0, "name": "Test1" },
        "right": { "character": REMILIA, "deck": [0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4], "palette": 1, "name": "Test2" },
        "stage": 0,
        "music": 0,
        "vs_com": False
    }
})

for _ in pbar:
    init_td = env.reset(init_params)
    env.set_health(100, 100)
    env.set_weather(CLEAR, 0, True)
    env.set_restricted_moves([200, 201])
    rollout = env.rollout(3600, policy, tensordict=init_td, auto_reset=False)
    traj_return = rollout["next", "reward", 0].mean()
    (-traj_return).backward()
    gn = nn.utils.clip_grad_norm_(net.parameters(), 1.0)
    optimizer.step()
    optimizer.zero_grad()
    pbar.set_description(
        f"reward: {traj_return: 4.4f}, "
        f"last reward: {rollout[..., -1]['next', 'reward', 0].mean(): 4.4f}, gradient norm: {gn: 4.4}"
    )
    logs["return"].append(traj_return.item())
    logs["last_reward"].append(rollout[..., -1]["next", "reward", 0].mean().item())
    scheduler.step()
