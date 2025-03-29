import random

import torch
import math
from tensordict import TensorDict, TensorDictBase

from torchrl.data import Bounded, Composite, Unbounded, Binary, NonTensor
from torchrl.envs import EnvBase
from .weathers import AURORA, MOUNTAIN_VAPOR, CLEAR
from .characters import LAST_CHARACTER
from .GameInstance import GameInstance, GameEndedException, GameCrashedError


CRASH_PENALTY = -1000000
WIN_BONUS = 400000
LOSE_PENALTY = -400000
DRAW_PENALTY = -200000
HP_LOSS_PENALTY = -10
ROUND_WIN_BONUS = 10000
ROUND_LOSS_PENALTY = -10000
DAMAGE_BONUS = 20
IDLE_PENALTY_FORMULA = lambda t: -math.exp(t // 60 - 10) - 1


def compute_reward(old_state, new_state):
    l_reward = IDLE_PENALTY_FORMULA(new_state["left"]["time_idle"])
    r_reward = IDLE_PENALTY_FORMULA(new_state["right"]["time_idle"])

    if old_state["left"]["score"] != new_state["left"]["score"]:
        l_reward += (new_state["left"]["score"] - old_state["left"]["score"]) * ROUND_LOSS_PENALTY
        r_reward += (new_state["left"]["score"] - old_state["left"]["score"]) * ROUND_WIN_BONUS
    if old_state["right"]["score"] != new_state["right"]["score"]:
        l_reward += (new_state["right"]["score"] - old_state["right"]["score"]) * ROUND_WIN_BONUS
        r_reward += (new_state["right"]["score"] - old_state["right"]["score"]) * ROUND_LOSS_PENALTY

    if old_state["left"]["hp"] != new_state["left"]["hp"]:
        l_reward += (old_state["left"]["hp"] - new_state["left"]["hp"]) * HP_LOSS_PENALTY
        r_reward += (old_state["left"]["hp"] - new_state["left"]["hp"]) * DAMAGE_BONUS
    if old_state["right"]["hp"] != new_state["right"]["hp"]:
        l_reward += (old_state["right"]["hp"] - new_state["right"]["hp"]) * DAMAGE_BONUS
        r_reward += (old_state["right"]["hp"] - new_state["right"]["hp"]) * HP_LOSS_PENALTY
    return l_reward, r_reward


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


def tensor_from_state(old, frame, params, shape):
    weathers = frame["weather"]
    left  = post_process_player(None if old is None else old["left"], frame["right"], frame["left"],  weathers)
    right = post_process_player(None if old is None else old["right"], frame["left"], frame["right"], weathers)
    if weathers["displayed"] == AURORA and weathers["active"] != CLEAR:
        weathers["active"] = AURORA
    return TensorDict({
        "params": TensorDict({
            "left":  TensorDict({ "character": params["left"]["character"] }),
            "right": TensorDict({ "character": params["right"]["character"] }),
        }),
        "left": TensorDict({
            "hp": left["hp"],
            "opponentRelativePos.x": left["opponentRelativePos.x"],
            "opponentRelativePos.y": left["opponentRelativePos.y"],
            "score": left["score"],
            "action": left["action"],
            "pose": left["pose"],
            "poseFrame": left["poseFrame"],
            "frameCount": left["frameCount"],
            "comboDamage": left["comboDamage"],
            "time_idle": left["time_idle"],
        }),
        "right": TensorDict({
            "hp": right["hp"],
            "opponentRelativePos.x": right["opponentRelativePos.x"],
            "opponentRelativePos.y": right["opponentRelativePos.y"],
            "score": right["score"],
            "action": right["action"],
            "pose": right["pose"],
            "poseFrame": right["poseFrame"],
            "frameCount": right["frameCount"],
            "comboDamage": right["comboDamage"],
            "time_idle": right["time_idle"],
        }),
        "weather": TensorDict({
            "active": weathers["active"],
            "displayed": weathers["displayed"],
            "timer": weathers["timer"],
        })
    }).unsqueeze(-1)


class SokuEnv(GameInstance, EnvBase):
    playing = False
    batch_locked = False
    _allow_done_after_reset = False
    _has_dynamic_specs = True
    force_health = None

    def __init__(self, exe_path, port=10800, seed=None, device="cpu"):
        EnvBase.__init__(self, device=device, batch_size=torch.Size())
        GameInstance.__init__(self, exe_path, port, just_connect=False)
        self._make_spec()
        if seed is None:
            seed = torch.empty((), dtype=torch.int64).random_().item()
        self.set_seed(seed)
        self.wait_for_ready()

    def _step(self, tensordict):
        if self.last_state["done"]:
            return self.last_state
        assert self.playing
        try:
            tensordict = tensordict[0]
            action = list(tensordict["action"]) + [0] * 10
            frame = self.tick({
                "left":  {"A": action[0],  "B": action[1],  "C": action[2],  "D": action[3],  "SC": action[4],  "SW": action[5],  "H": action[6]  - action[7],  "V": action[8]  - action[9]},
                "right": {"A": action[10], "B": action[11], "C": action[12], "D": action[13], "SC": action[14], "SW": action[15], "H": action[16] - action[17], "V": action[18] - action[19]}
            })
            if self.force_health:
                mod = False
                if frame["left"].hp > self.force_health[0]:
                    self.last_state["left"]["hp"] = self.force_health[0]
                    mod = True
                if frame["right"].hp > self.force_health[1]:
                    self.last_state["right"]["hp"] = self.force_health[1]
                    mod = True
                if mod:
                    GameInstance.set_health(self, self.last_state["left"]["hp"], self.last_state["right"]["hp"])
            new_state = tensor_from_state(None, frame, self.last_state["params"], tensordict.shape)
            reward = compute_reward(self.last_state, new_state)
            new_state["done"] = [False]
            new_state["reward"] = [reward[0]]
            self.last_state = new_state
            return self.last_state
        except GameEndedException as e:
            if e.winner == 0:
                self.last_state["reward"] = [DRAW_PENALTY]
            elif e.winner == 1:
                self.last_state["reward"] = [WIN_BONUS]
            else:
                self.last_state["reward"] = [LOSE_PENALTY]
            self.last_state["done"] = [True]
            return self.last_state
        except GameCrashedError:
            self.last_state["reward"] = [CRASH_PENALTY]
            self.last_state["done"] = [True]
            self.reconnect()
            return self.last_state

    def _reset(self, tensordict: TensorDictBase, **kwargs):
        if tensordict is None:
            tensordict = TensorDict({
                "params": {
                    "left": { "character": random.choice(range(0, 20)), "deck": [0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4], "palette": 0, "name": "Empty" },
                    "right": { "character": random.choice(range(0, 20)), "deck": [0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4], "palette": 1, "name": "Empty" },
                    "stage": 0,
                    "music": 0,
                    "vs_com": False
                }
            })
        self.wait_for_ready()
        self.start_game(tensordict[0]["params"], tensordict[0]["params"]["vs_com"])
        self.last_state = tensor_from_state(None, self.tick({ "left": self.EMPTY_INPUT, "right": self.EMPTY_INPUT }), tensordict[0]["params"], tensordict.shape)
        self.last_state[0]["reward"] = [[0, 0]]
        self.last_state[0]["done"] = [False]
        self.playing = True
        return self.last_state

    def set_health(self, left, right):
        GameInstance.set_health(self, left, right)
        self.force_health = [left, right]
        self.last_state["left"]["hp"] = [left]
        self.last_state["right"]["hp"] = [right]

    def set_positions(self, left, right):
        GameInstance.set_positions(self, left, right)
        self.last_state["left"]["position.x"] = [left["x"]]
        self.last_state["left"]["position.y"] = [left["y"]]
        self.last_state["right"]["position.x"] = [right["x"]]
        self.last_state["right"]["position.y"] = [right["y"]]

    def set_weather(self, weather, timer, freeze_timer=True):
        GameInstance.set_weather(self, weather, timer, freeze_timer)
        self.last_state["weather"]["timer"] = [timer]
        self.last_state["weather"]["active"] = [weather]
        self.last_state["weather"]["displayed"] = [weather]


    def _make_spec(self):
        param_composite = Composite(
            character=Bounded(low=0, high=LAST_CHARACTER, dtype=torch.int64, shape=1),
            deck=Unbounded(dtype=torch.int64, shape=20)
        )
        state_composite = Composite(
            {
                "opponentRelativePos.x": Bounded(low=-900, high=2200, shape=1),
                "opponentRelativePos.y": Bounded(low=-100, high=2200, shape=1)
            },
            score=Bounded(low=0, high=2, dtype=torch.int64, shape=1),
            hp=Bounded(low=0, high=10000, dtype=torch.int64, shape=1),
            action=Bounded(low=0, high=799, dtype=torch.int64, shape=1),
            pose=Unbounded(dtype=torch.int64, shape=1),
            poseFrame=Unbounded(dtype=torch.int64, shape=1),
            frameCount=Unbounded(dtype=torch.int64, shape=1),
            comboDamage=Unbounded(dtype=torch.int64, shape=1),
            time_idle=Unbounded(dtype=torch.int64, shape=1)
        )
        """state_composite = Composite(
            {
                "position.x": Bounded(low=-900, high=2200, shape=1),
                "position.y": Bounded(low=-100, high=2200, shape=1),
                "opponentRelativePos.x": Bounded(low=-900, high=2200, shape=1),
                "opponentRelativePos.y": Bounded(low=-100, high=2200, shape=1)
            },
            direction=Bounded(low=-1, high=1, dtype=torch.uint8, shape=1),
            action=Bounded(low=0, high=799, dtype=torch.int64, shape=1),
            sequence=Unbounded(dtype=torch.int64, shape=1),
            pose=Unbounded(dtype=torch.int64, shape=1),
            poseFrame=Unbounded(dtype=torch.int64, shape=1),
            frameCount=Unbounded(dtype=torch.int64, shape=1),
            comboDamage=Unbounded(dtype=torch.int64, shape=1),
            comboLimit=Unbounded(dtype=torch.int64, shape=1),
            airBorne=Unbounded(dtype=torch.bool, shape=1),
            timeStop=Unbounded(dtype=torch.int64, shape=1),
            hitStop=Unbounded(dtype=torch.int64, shape=1),
            hp=Bounded(low=0, high=10000, dtype=torch.int64, shape=1),
            airDashCount=Bounded(low=0, high=3, dtype=torch.uint8, shape=1),
            spirit=Bounded(low=0, high=1000, dtype=torch.int64, shape=1),
            maxSpirit=Bounded(low=0, high=1000, dtype=torch.int64, shape=1),
            untech=Unbounded(dtype=torch.int64, shape=1),
            healingCharm=Bounded(low=0, high=250, dtype=torch.int64, shape=1),
            confusion=Bounded(low=0, high=900, dtype=torch.int64, shape=1),
            swordOfRapture=Bounded(low=0, high=900, dtype=torch.int64, shape=1),
            score=Bounded(low=0, high=2, dtype=torch.uint8, shape=1),
            hand=Composite(
                cost=Bounded(low=-2, high=5, dtype=torch.int8, shape=1),
                id=Bounded(low=-2, high=220, dtype=torch.int64, shape=1)
            ),
            cardGauge=Bounded(low=0, high=900, dtype=torch.int64, shape=1),
            skills=Bounded(low=0, high=4, dtype=torch.uint8, shape=15),
            fanLevel=Bounded(low=0, high=4, dtype=torch.uint8, shape=1),
            rodsLevel=Bounded(low=0, high=4, dtype=torch.uint8, shape=1),
            booksLevel=Bounded(low=0, high=4, dtype=torch.uint8, shape=1),
            dollLevel=Bounded(low=0, high=4, dtype=torch.uint8, shape=1),
            dropLevel=Bounded(low=0, high=3, dtype=torch.uint8, shape=1),
            dropInvulTimeLeft=Bounded(low=0, high=1200, dtype=torch.int64, shape=1),
            superArmorHp=Bounded(low=-10000, high=10000, dtype=torch.int16, shape=1),
            image=Bounded(low=0, high=2000, dtype=torch.int64, shape=1),
            distToLeftCorner=Bounded(low=-900, high=2200, dtype=torch.int64, shape=1),
            distToRightCorner=Bounded(low=-900, high=2200, dtype=torch.int64, shape=1),
            distToFrontCorner=Bounded(low=-900, high=2200, dtype=torch.int64, shape=1),
            distToBackCorner=Bounded(low=-900, high=2200, dtype=torch.int64, shape=1),
            objects=Composite(
                {
                    "position.x": Bounded(low=-900, high=2200, shape=1),
                    "position.y": Bounded(low=-100, high=2200, shape=1)
                },
                direction=Bounded(low=-1, high=1, dtype=torch.uint8, shape=1),
                action=Bounded(low=0, high=799, dtype=torch.int64, shape=1),
                sequence=Unbounded(dtype=torch.int64, shape=1),
                hitStop=Unbounded(dtype=torch.int64, shape=1),
                image=Bounded(low=0, high=2000, dtype=torch.int64, shape=1)
            ),
            time_idle=Unbounded(dtype=torch.int64, shape=1)
        )"""

        # Under the hood, this will populate self.output_spec["observation"]
        self.observation_spec = Composite(
            params=Composite(
                left=param_composite.clone(),
                right=param_composite
            ),
            left=state_composite.clone(),
            right=state_composite,
            weather=Composite(
                timer=Bounded(low=0, high=999, dtype=torch.int64, shape=1),
                active=Bounded(low=0, high=21, dtype=torch.uint8, shape=1),
                display=Bounded(low=0, high=21, dtype=torch.uint8, shape=1)
            )
        )
        # since the environment is stateless, we expect the previous output as input.
        # For this, ``EnvBase`` expects some state_spec to be available
        self.state_spec = Composite(
            params=Composite(
                left=Composite(
                    character=Bounded(low=0, high=LAST_CHARACTER, dtype=torch.int64, shape=1),
                    palette=Bounded(low=0, high=7, dtype=torch.int64, shape=1),
                    deck=Unbounded(dtype=torch.int64, shape=20),
                    name=NonTensor()
                ),
                right= Composite(
                    character=Bounded(low=0, high=LAST_CHARACTER, dtype=torch.int64, shape=1),
                    palette=Bounded(low=0, high=7, dtype=torch.int64, shape=1),
                    deck=Unbounded(dtype=torch.int64, shape=20),
                    name=NonTensor()
                ),
                stage=Unbounded(dtype=torch.int64),
                music=Unbounded(dtype=torch.int64),
                vs_com=Unbounded(dtype=torch.bool)
            )
        )
        # action-spec will be automatically wrapped in input_spec when
        # `self.action_spec = spec` will be called supported
        self.action_spec = Binary(shape=torch.Size([20]))
        self.reward_spec = Unbounded(dtype=torch.float64, shape=torch.Size([2]))

    def _set_seed(self, seed: int | None):
        rng = torch.manual_seed(seed)
        self.rng = rng
