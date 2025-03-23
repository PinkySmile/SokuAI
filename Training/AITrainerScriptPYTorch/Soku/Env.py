from collections import defaultdict
from typing import Optional

import numpy as np
import torch
import tqdm
import math
from tensordict import TensorDict, TensorDictBase
from tensordict.nn import TensorDictModule
from torch import nn

from torchrl.data import Bounded, Composite, Unbounded, Binary
from torchrl.envs import (
    CatTensors,
    EnvBase,
    Transform,
    TransformedEnv,
    UnsqueezeTransform,
)
from torchrl.envs.transforms.transforms import _apply_to_composite
from torchrl.envs.utils import check_env_specs, step_mdp, make_composite_from_td
from .weathers import AURORA, MOUNTAIN_VAPOR, CLEAR
from .characters import REMILIA, LAST_CHARACTER
from .GameInstance import GameInstance, GameEndedException, GameCrashedError


def post_process_player(old, opp, me, weathers):
    d = me.dict()
    if weathers["active"] == MOUNTAIN_VAPOR:
        d["hand"] = [{'id': -2, 'cost': -2} for _ in range(5)]
    if old is not None and 50 > me.action >= 200 and me.action < 300:
        d["time_idle"] = old["time_idle"] + 1
    else:
        d["time_idle"] = 0
    d["hand"] = [{'id': f['id'] if f['id'] != 255 else -1, 'cost': f['cost'] if f['cost'] != 255 else -1} for f in d["hand"]]
    return d


def tensor_from_state(old, frame, shape):
    weathers = frame["weather"]
    left  = post_process_player(None if old is None else old["left"], frame["right"], frame["left"],  weathers)
    right = post_process_player(None if old is None else old["right"], frame["left"], frame["right"], weathers)
    if weathers["displayed"] == AURORA and weathers["active"] != CLEAR:
        weathers["active"] = AURORA
    return TensorDict({ "left": left, "right": right, "weather": weathers }, batch_size=shape)


def compute_reward(old_state, new_state):
    l_reward = -math.exp(new_state["left"]["time_idle"] // 60 - 10) - 1
    r_reward = -math.exp(new_state["right"]["time_idle"] // 60 - 10) - 1
    if old_state["left"]["hp"] != new_state["left"]["hp"]:
        l_reward -= (old_state["left"]["hp"] - new_state["left"]["hp"]) * 10
        r_reward += (old_state["left"]["hp"] - new_state["left"]["hp"]) * 20
    if old_state["right"]["hp"] != new_state["right"]["hp"]:
        l_reward += (old_state["right"]["hp"] - new_state["right"]["hp"]) * 20
        r_reward -= (old_state["right"]["hp"] - new_state["right"]["hp"]) * 10
    return l_reward, r_reward


class SokuEnv(GameInstance, EnvBase):
    playing = False
    batch_locked = False
    _allow_done_after_reset = False

    def __init__(self, exe_path, port=10800, seed=None, device="cpu"):
        EnvBase.__init__(self, device=device, batch_size=torch.Size())
        GameInstance.__init__(self, exe_path, port, just_connect=False)
        self._make_spec()
        if seed is None:
            seed = torch.empty((), dtype=torch.int64).random_().item()
        self.set_seed(seed)

    def _step(self, tensordict):
        assert self.playing
        try:
            frame = self.tick(tensordict["input"])
            new_state = tensor_from_state(None, frame, tensordict.shape)
            reward = compute_reward(self.last_state, new_state)
            new_state["params"] = self.last_state["params"]
            new_state["done"] = False
            new_state["reward_left"] = reward[0]
            new_state["reward_right"] = reward[1]
            self.last_state = new_state
            return self.last_state
        except GameEndedException as e:
            if e.winner == 0:
                self.last_state["reward_left"]  = -200000
                self.last_state["reward_right"] = -200000
            elif e.winner == 1:
                self.last_state["reward_left"]  =  400000
                self.last_state["reward_right"] = -400000
            else:
                self.last_state["reward_left"]  = -400000
                self.last_state["reward_right"] =  400000
            self.last_state["done"] = True
            return self.last_state
        except GameCrashedError:
            self.last_state["reward_left"] = -1000000
            self.last_state["reward_right"] = -1000000
            self.last_state["done"] = True
            self.reconnect()
            return self.last_state

    def _reset(self, tensordict: TensorDictBase, **kwargs):
        if self.playing:
            try:
                EMPTY_INPUT = {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": 0, "V": 0}
                while True:
                    self.end_game()
                    self.tick({ "left": EMPTY_INPUT, "right": EMPTY_INPUT })
            except GameEndedException:
                self.playing = False
                pass
        self.last_state = tensor_from_state(None, self.start_game(tensordict["params"], tensordict["params"]["vs_com"]), tensordict.shape)
        self.last_state["params"] = tensordict["params"]
        self.last_state["reward_left"] = 0
        self.last_state["reward_right"] = 0
        self.last_state["done"] = False
        self.playing = True
        return self.last_state

    def _make_spec(self):
        param_composite = Composite(
            character=Bounded(low=0, high=LAST_CHARACTER, dtype=torch.uint8, shape=1),
            deck=Unbounded(dtype=torch.uint8, shape=20)
        )
        state_composite = Composite(
            {
                "position.x": Bounded(low=-900, high=2200, shape=1),
                "position.y": Bounded(low=-100, high=2200, shape=1),
                "opponentRelativePos.x": Bounded(low=-900, high=2200, shape=1),
                "opponentRelativePos.y": Bounded(low=-100, high=2200, shape=1)
            },
            direction=Bounded(low=-1, high=1, dtype=torch.uint8, shape=1),
            action=Bounded(low=0, high=799, dtype=torch.uint16, shape=1),
            sequence=Unbounded(dtype=torch.uint16, shape=1),
            pose=Unbounded(dtype=torch.uint16, shape=1),
            poseFrame=Unbounded(dtype=torch.uint32, shape=1),
            comboDamage=Unbounded(dtype=torch.uint16, shape=1),
            comboLimit=Unbounded(dtype=torch.uint16, shape=1),
            airBorne=Unbounded(dtype=torch.bool, shape=1),
            timeStop=Unbounded(dtype=torch.uint16, shape=1),
            hitStop=Unbounded(dtype=torch.uint16, shape=1),
            hp=Bounded(low=0, high=10000, dtype=torch.uint16, shape=1),
            airDashCount=Bounded(low=0, high=3, dtype=torch.uint8, shape=1),
            spirit=Bounded(low=0, high=1000, dtype=torch.uint16, shape=1),
            maxSpirit=Bounded(low=0, high=1000, dtype=torch.uint16, shape=1),
            untech=Unbounded(dtype=torch.uint16, shape=1),
            healingCharm=Bounded(low=0, high=250, dtype=torch.uint16, shape=1),
            confusion=Bounded(low=0, high=900, dtype=torch.uint16, shape=1),
            swordOfRapture=Bounded(low=0, high=900, dtype=torch.uint16, shape=1),
            score=Bounded(low=0, high=2, dtype=torch.uint8, shape=1),
            hand=Composite(
                cost=Bounded(low=-2, high=5, dtype=torch.int8, shape=1),
                id=Bounded(low=-2, high=220, dtype=torch.uint16, shape=1)
            ),
            cardGauge=Bounded(low=0, high=900, dtype=torch.uint16, shape=1),
            skills=Bounded(low=0, high=4, dtype=torch.uint8, shape=15),
            fanLevel=Bounded(low=0, high=4, dtype=torch.uint8, shape=1),
            rodsLevel=Bounded(low=0, high=4, dtype=torch.uint8, shape=1),
            booksLevel=Bounded(low=0, high=4, dtype=torch.uint8, shape=1),
            dollLevel=Bounded(low=0, high=4, dtype=torch.uint8, shape=1),
            dropLevel=Bounded(low=0, high=3, dtype=torch.uint8, shape=1),
            dropInvulTimeLeft=Bounded(low=0, high=1200, dtype=torch.uint16, shape=1),
            superArmorHp=Bounded(low=-10000, high=10000, dtype=torch.int16, shape=1),
            image=Bounded(low=0, high=2000, dtype=torch.uint16, shape=1),
            distToLeftCorner=Bounded(low=-900, high=2200, dtype=torch.uint16, shape=1),
            distToRightCorner=Bounded(low=-900, high=2200, dtype=torch.uint16, shape=1),
            distToFrontCorner=Bounded(low=-900, high=2200, dtype=torch.uint16, shape=1),
            distToBackCorner=Bounded(low=-900, high=2200, dtype=torch.uint16, shape=1),
            objects=Composite(
                {
                    "position.x": Bounded(low=-900, high=2200, shape=1),
                    "position.y": Bounded(low=-100, high=2200, shape=1)
                },
                direction=Bounded(low=-1, high=1, dtype=torch.uint8, shape=1),
                action=Bounded(low=0, high=799, dtype=torch.uint16, shape=1),
                sequence=Unbounded(dtype=torch.uint16, shape=1),
                hitStop=Unbounded(dtype=torch.uint16, shape=1),
                image=Bounded(low=0, high=2000, dtype=torch.uint16, shape=1)
            ),
            time_idle=Unbounded(dtype=torch.uint16, shape=1)
        )

        # Under the hood, this will populate self.output_spec["observation"]
        self.observation_spec = Composite(
            params=Composite(
                left=param_composite.clone(),
                right=param_composite
            ),
            left=state_composite.clone(),
            right=state_composite
        )
        # since the environment is stateless, we expect the previous output as input.
        # For this, ``EnvBase`` expects some state_spec to be available
        self.state_spec = self.observation_spec.clone()
        # action-spec will be automatically wrapped in input_spec when
        # `self.action_spec = spec` will be called supported
        self.action_spec = Binary(shape=torch.Size([20]))
        self.reward_spec = Unbounded(shape=torch.Size([1]))

    def _set_seed(self, seed: int | None):
        rng = torch.manual_seed(seed)
        self.rng = rng


env = SokuEnv('/home/pinky/Bureau/gamePool/0/th123.exe', 10801)
env.wait_for_ready()
check_env_specs(env)
print("observation_spec:", env.observation_spec)
print("state_spec:", env.state_spec)
print("reward_spec:", env.reward_spec)
td = env.reset(TensorDict({
    "params": {
        "left": { "character": REMILIA, "deck": [0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4], "palette": 0, "name": "Test1" },
        "right": { "character": REMILIA, "deck": [0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4], "palette": 1, "name": "Test2" },
        "stage": 0,
        "music": 0,
        "vs_com": True
    }
}))
print("reset tensordict", td)
td = env.rand_step(td)
print("random step tensordict", td)
