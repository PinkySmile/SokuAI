import random

import torch
import math
from tensordict import TensorDict, TensorDictBase

from torchrl.data import Bounded, Composite, Unbounded, Binary, NonTensor
from torchrl.envs import EnvBase
from .weathers import AURORA, MOUNTAIN_VAPOR, CLEAR
from .characters import LAST_CHARACTER
from .GameInstance import GameInstance, GameEndedException, GameCrashedError
from .BaseAI import BaseAI


CRASH_PENALTY = -1000000
WIN_BONUS = 400000
LOSE_PENALTY = -400000
DRAW_PENALTY = -200000
HP_LOSS_PENALTY = -10
ROUND_WIN_BONUS = 10000
ROUND_LOSS_PENALTY = -10000
DAMAGE_BONUS = 20
IDLE_PENALTY_FORMULA = lambda t: -math.exp(min((t - 300) / 30, 4)) - 1


def compute_reward(old_state, new_state):
    l_reward = IDLE_PENALTY_FORMULA(new_state["left"]["time_idle"])
    r_reward = IDLE_PENALTY_FORMULA(new_state["right"]["time_idle"])

    if old_state["left"]["score"] != new_state["left"]["score"]:
        l_reward += (new_state["left"]["score"] - old_state["left"]["score"]) * ROUND_WIN_BONUS
        r_reward += (new_state["left"]["score"] - old_state["left"]["score"]) * ROUND_LOSS_PENALTY
    if old_state["right"]["score"] != new_state["right"]["score"]:
        l_reward += (new_state["right"]["score"] - old_state["right"]["score"]) * ROUND_LOSS_PENALTY
        r_reward += (new_state["right"]["score"] - old_state["right"]["score"]) * ROUND_WIN_BONUS

    if old_state["left"]["hp"] < new_state["left"]["hp"]:
        l_reward += (old_state["left"]["hp"] - new_state["left"]["hp"]) * HP_LOSS_PENALTY
        r_reward += (old_state["left"]["hp"] - new_state["left"]["hp"]) * DAMAGE_BONUS
    if old_state["right"]["hp"] < new_state["right"]["hp"]:
        l_reward += (old_state["right"]["hp"] - new_state["right"]["hp"]) * DAMAGE_BONUS
        r_reward += (old_state["right"]["hp"] - new_state["right"]["hp"]) * HP_LOSS_PENALTY
    return l_reward, r_reward


def post_process_player(old, opp, me, weathers):
    d = me.dict()
    if weathers["active"] == MOUNTAIN_VAPOR:
        d["hand"] = [{'id': -2, 'cost': -2} for _ in range(5)]
    if old is not None and (me.action < 50 or 200 <= me.action < 220):
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
    return {
        "params": {
            "left":  { "character": params["left"]["character"] },
            "right": { "character": params["right"]["character"] },
        },
        "left": {
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
        },
        "right": {
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
        },
        "weather": {
            "active": weathers["active"],
            "displayed": weathers["displayed"],
            "timer": weathers["timer"],
        }
    }


def tensorify(d):
    if isinstance(d, dict):
        return { key: tensorify(value) for key, value in d.items() }
    if isinstance(d, bool):
        return torch.tensor(d, dtype=torch.bool)
    return torch.tensor(d, dtype=torch.float32, requires_grad=True)


class SokuEnv(EnvBase):
    EMPTY_INPUT = {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": 0, "V": 0}
    playing = False
    batch_locked = False
    _allow_done_after_reset = False
    _has_dynamic_specs = True
    force_health = None
    p2ai: BaseAI = None

    def __init__(self, exe_path, max_games=1, start_port=10800, seed=None, device="cpu"):
        super().__init__(device=device, batch_size=torch.Size())
        self.instances = []
        for i in range(max_games):
            while True:
                try:
                    self.instances.append(GameInstance(exe_path, start_port, just_connect=False))
                    break
                except OSError:
                    start_port += 1
            start_port += 1
        self._make_spec()
        if seed is None:
            seed = torch.empty((), dtype=torch.int64).random_().item()
        self.set_seed(seed)
        self.wait_for_ready()

    def _step(self, tensordict):
        if not self.last_state["done"]:
            assert self.playing
            try:
                tensordict = tensordict[0]
                action = list(tensordict["action"]) + [0] * 10
                frame = self.instances[0].tick({
                    "left":  {
                        "A": bool(action[0] > 0),
                        "B": bool(action[1] > 0),
                        "C": bool(action[2] > 0),
                        "D": bool(action[3] > 0),
                        "SC": bool(action[4] > 0),
                        "SW": bool(action[5] > 0),
                        "H": bool(action[6] > 0) - bool(action[7] > 0),
                        "V": bool(action[8] > 0) - bool(action[9] > 0)
                    },
                    "right": {
                        "A": bool(action[10] > 0),
                        "B": bool(action[11] > 0),
                        "C": bool(action[12] > 0),
                        "D": bool(action[13] > 0),
                        "SC": bool(action[14] > 0),
                        "SW": bool(action[15] > 0),
                        "H": bool(action[16] > 0) - bool(action[17] > 0),
                        "V": bool(action[18] > 0) - bool(action[19] > 0)
                    }
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
                        self.instances[0].set_health(self.last_state["left"]["hp"], self.last_state["right"]["hp"])
                new_state = tensor_from_state(self.last_state, frame, self.last_state["params"], tensordict.shape)
                reward = compute_reward(self.last_state, new_state)
                new_state["done"] = False
                new_state["reward"] = reward[0]
                self.last_state = new_state
            except GameEndedException as e:
                if e.winner == 0:
                    self.last_state["reward"] = DRAW_PENALTY
                elif e.winner == 1:
                    self.last_state["reward"] = WIN_BONUS
                else:
                    self.last_state["reward"] = LOSE_PENALTY
                self.last_state["done"] = True
            except GameCrashedError:
                self.last_state["reward"] = CRASH_PENALTY
                self.last_state["done"] = True
                self.reconnect()
        return TensorDict(tensorify(self.last_state)).unsqueeze(-1)

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
            }).unsqueeze(-1)
        params = tensordict[0]["params"]
        dict_params = {
            "left":  { "character": int(params["left"]["character"]) },
            "right": { "character": int(params["right"]["character"]) }
        }
        self.wait_for_ready()
        self.instances[0].start_game(params, params["vs_com"])
        self.last_frame = self.instances[0].tick({ "left": self.EMPTY_INPUT, "right": self.EMPTY_INPUT })
        self.last_state = tensor_from_state(None, self.last_frame, dict_params, tensordict.shape)
        self.last_state["reward"] = 0
        self.last_state["done"] = False
        self.playing = True
        return TensorDict(self.last_state).unsqueeze(-1)

    def set_health(self, left, right):
        for instance in self.instances:
            instance.set_health(left, right)
        self.force_health = [left, right]
        self.last_state["left"]["hp"] = left
        self.last_state["right"]["hp"] = right

    def set_positions(self, left, right):
        for instance in self.instances:
            instance.set_positions(left, right)
        self.last_state["left"]["position.x"] = left["x"]
        self.last_state["left"]["position.y"] = left["y"]
        self.last_state["right"]["position.x"] = right["x"]
        self.last_state["right"]["position.y"] = right["y"]

    def set_weather(self, weather, timer, freeze_timer=True):
        for instance in self.instances:
            instance.set_weather(weather, timer, freeze_timer=freeze_timer)
        self.last_state["weather"]["timer"] = timer
        self.last_state["weather"]["active"] = weather
        self.last_state["weather"]["displayed"] = weather

    def wait_for_ready(self):
        for instance in self.instances:
            instance.wait_for_ready()

    def set_game_speed(self, tps):
        for instance in self.instances:
            instance.set_game_speed(tps)

    def set_display_mode(self, shown=True):
        for instance in self.instances:
            instance.set_display_mode(shown=shown)

    def set_game_volume(self, sfx, bgm):
        for instance in self.instances:
            instance.set_game_volume(sfx, bgm)

    def set_restricted_moves(self, moves):
        for instance in self.instances:
            instance.set_restricted_moves(moves)

    def _make_spec(self):
        param_composite = Composite(
            character=Bounded(low=0, high=LAST_CHARACTER, dtype=torch.float32, shape=1),
            deck=Unbounded(dtype=torch.float32, shape=20)
        )
        state_composite = Composite(
            {
                "opponentRelativePos.x": Bounded(low=-900, high=2200, dtype=torch.float32, shape=1),
                "opponentRelativePos.y": Bounded(low=-100, high=2200, dtype=torch.float32, shape=1)
            },
            score=Bounded(low=0, high=2, dtype=torch.float32, shape=1),
            hp=Bounded(low=0, high=10000, dtype=torch.float32, shape=1),
            action=Bounded(low=0, high=799, dtype=torch.float32, shape=1),
            pose=Unbounded(dtype=torch.float32, shape=1),
            poseFrame=Unbounded(dtype=torch.float32, shape=1),
            frameCount=Unbounded(dtype=torch.float32, shape=1),
            comboDamage=Unbounded(dtype=torch.float32, shape=1),
            time_idle=Unbounded(dtype=torch.float32, shape=1)
        )
        """state_composite = Composite(
            {
                "position.x": Bounded(low=-900, high=2200, shape=1),
                "position.y": Bounded(low=-100, high=2200, shape=1),
                "opponentRelativePos.x": Bounded(low=-900, high=2200, shape=1),
                "opponentRelativePos.y": Bounded(low=-100, high=2200, shape=1)
            },
            direction=Bounded(low=-1, high=1, dtype=torch.uint8, shape=1),
            action=Bounded(low=0, high=799, dtype=torch.float32, shape=1),
            sequence=Unbounded(dtype=torch.float32, shape=1),
            pose=Unbounded(dtype=torch.float32, shape=1),
            poseFrame=Unbounded(dtype=torch.float32, shape=1),
            frameCount=Unbounded(dtype=torch.float32, shape=1),
            comboDamage=Unbounded(dtype=torch.float32, shape=1),
            comboLimit=Unbounded(dtype=torch.float32, shape=1),
            airBorne=Unbounded(dtype=torch.bool, shape=1),
            timeStop=Unbounded(dtype=torch.float32, shape=1),
            hitStop=Unbounded(dtype=torch.float32, shape=1),
            hp=Bounded(low=0, high=10000, dtype=torch.float32, shape=1),
            airDashCount=Bounded(low=0, high=3, dtype=torch.uint8, shape=1),
            spirit=Bounded(low=0, high=1000, dtype=torch.float32, shape=1),
            maxSpirit=Bounded(low=0, high=1000, dtype=torch.float32, shape=1),
            untech=Unbounded(dtype=torch.float32, shape=1),
            healingCharm=Bounded(low=0, high=250, dtype=torch.float32, shape=1),
            confusion=Bounded(low=0, high=900, dtype=torch.float32, shape=1),
            swordOfRapture=Bounded(low=0, high=900, dtype=torch.float32, shape=1),
            score=Bounded(low=0, high=2, dtype=torch.uint8, shape=1),
            hand=Composite(
                cost=Bounded(low=-2, high=5, dtype=torch.int8, shape=1),
                id=Bounded(low=-2, high=220, dtype=torch.float32, shape=1)
            ),
            cardGauge=Bounded(low=0, high=900, dtype=torch.float32, shape=1),
            skills=Bounded(low=0, high=4, dtype=torch.uint8, shape=15),
            fanLevel=Bounded(low=0, high=4, dtype=torch.uint8, shape=1),
            rodsLevel=Bounded(low=0, high=4, dtype=torch.uint8, shape=1),
            booksLevel=Bounded(low=0, high=4, dtype=torch.uint8, shape=1),
            dollLevel=Bounded(low=0, high=4, dtype=torch.uint8, shape=1),
            dropLevel=Bounded(low=0, high=3, dtype=torch.uint8, shape=1),
            dropInvulTimeLeft=Bounded(low=0, high=1200, dtype=torch.float32, shape=1),
            superArmorHp=Bounded(low=-10000, high=10000, dtype=torch.int16, shape=1),
            image=Bounded(low=0, high=2000, dtype=torch.float32, shape=1),
            distToLeftCorner=Bounded(low=-900, high=2200, dtype=torch.float32, shape=1),
            distToRightCorner=Bounded(low=-900, high=2200, dtype=torch.float32, shape=1),
            distToFrontCorner=Bounded(low=-900, high=2200, dtype=torch.float32, shape=1),
            distToBackCorner=Bounded(low=-900, high=2200, dtype=torch.float32, shape=1),
            objects=Composite(
                {
                    "position.x": Bounded(low=-900, high=2200, shape=1),
                    "position.y": Bounded(low=-100, high=2200, shape=1)
                },
                direction=Bounded(low=-1, high=1, dtype=torch.uint8, shape=1),
                action=Bounded(low=0, high=799, dtype=torch.float32, shape=1),
                sequence=Unbounded(dtype=torch.float32, shape=1),
                hitStop=Unbounded(dtype=torch.float32, shape=1),
                image=Bounded(low=0, high=2000, dtype=torch.float32, shape=1)
            ),
            time_idle=Unbounded(dtype=torch.float32, shape=1)
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
                timer=Bounded(low=0, high=999, dtype=torch.float32, shape=1),
                active=Bounded(low=0, high=21, dtype=torch.uint8, shape=1),
                display=Bounded(low=0, high=21, dtype=torch.uint8, shape=1)
            )
        )
        # since the environment is stateless, we expect the previous output as input.
        # For this, ``EnvBase`` expects some state_spec to be available
        self.state_spec = Composite(
            params=Composite(
                left=Composite(
                    character=Bounded(low=0, high=LAST_CHARACTER, dtype=torch.float32, shape=1),
                    palette=Bounded(low=0, high=7, dtype=torch.float32, shape=1),
                    deck=Unbounded(dtype=torch.float32, shape=20),
                    name=NonTensor()
                ),
                right= Composite(
                    character=Bounded(low=0, high=LAST_CHARACTER, dtype=torch.float32, shape=1),
                    palette=Bounded(low=0, high=7, dtype=torch.float32, shape=1),
                    deck=Unbounded(dtype=torch.float32, shape=20),
                    name=NonTensor()
                ),
                stage=Unbounded(dtype=torch.float32),
                music=Unbounded(dtype=torch.float32),
                vs_com=Unbounded(dtype=torch.bool)
            )
        )
        # action-spec will be automatically wrapped in input_spec when
        # `self.action_spec = spec` will be called supported
        self.action_spec = Binary(shape=torch.Size([10]))
        self.reward_spec = Unbounded(dtype=torch.float64, shape=torch.Size([1]))

    def _set_seed(self, seed: int | None):
        rng = torch.manual_seed(seed)
        self.rng = rng


class MoveBasedSokuEnv(SokuEnv):
    DOWN = 1
    UP = -1
    BACK = -1
    FRONT = 1
    NEUTRAL = 0
    EMPTY = {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": NEUTRAL}
    actions_buffers = {
        "1a": [
            {"A": True, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": DOWN}
        ],
        "2a": [
            {"A": True, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN}
        ],
        "3a": [
            {"A": True, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": DOWN}
        ],
        "4a": [
            {"A": True, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL}
        ],
        "5a": [
            {"A": True, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": NEUTRAL}
        ],
        "6a": [
            {"A": True, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL}
        ],
        "8a": [
            {"A": True, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": NEUTRAL}
        ],
        "66a": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
            EMPTY,
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
            {"A": True, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
        ],
        "2b": [
            {"A": False, "B": True, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN}
        ],
        "3b": [
            {"A": False, "B": True, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": DOWN}
        ],
        "4b": [
            {"A": False, "B": True, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL}
        ],
        "5b": [
            {"A": False, "B": True, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": NEUTRAL}
        ],
        "6b": [
            {"A": False, "B": True, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL}
        ],
        "66b": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
            EMPTY,
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
            {"A": False, "B": True, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
        ],
        "1c": [
            {"A": False, "B": False, "C": True, "D": False, "SC": False, "SW": False, "H": BACK, "V": DOWN}
        ],
        "2c": [
            {"A": False, "B": False, "C": True, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN}
        ],
        "3c": [
            {"A": False, "B": False, "C": True, "D": False, "SC": False, "SW": False, "H": FRONT, "V": DOWN}
        ],
        "4c": [
            {"A": False, "B": False, "C": True, "D": False, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL}
        ],
        "5c": [
            {"A": False, "B": False, "C": True, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": NEUTRAL}
        ],
        "6c": [
            {"A": False, "B": False, "C": True, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL}
        ],
        "66c": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
            EMPTY,
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
            {"A": False, "B": False, "C": True, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
        ],
        "236b": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": DOWN},
            {"A": False, "B": True, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
        ],
        "236c": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": DOWN},
            {"A": False, "B": False, "C": True, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
        ],
        "623b": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
            {"A": False, "B": True, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": DOWN},
        ],
        "623c": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
            {"A": False, "B": False, "C": True, "D": False, "SC": False, "SW": False, "H": FRONT, "V": DOWN},
        ],
        "214b": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": DOWN},
            {"A": False, "B": True, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL},
        ],
        "214c": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": DOWN},
            {"A": False, "B": False, "C": True, "D": False, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL},
        ],
        "421b": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL},
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
            {"A": False, "B": True, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": DOWN},
        ],
        "421c": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL},
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
            {"A": False, "B": False, "C": True, "D": False, "SC": False, "SW": False, "H": BACK, "V": DOWN},
        ],
        "22b": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
            EMPTY,
            {"A": False, "B": True, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
        ],
        "22c": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
            EMPTY,
            {"A": False, "B": False, "C": True, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
        ],
        "nothing": [EMPTY],
        "forward": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL}
        ],
        "backward": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL}
        ],
        "block_low": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": DOWN}
        ],
        "forward_jump": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": UP}
        ],
        "backward_jump": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": UP}
        ],
        "neutral_jump": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": UP}
        ],
        "forward_highjump": [
            {"A": False, "B": False, "C": False, "D": True, "SC": False, "SW": False, "H": FRONT, "V": UP}
        ],
        "backward_highjump": [
            {"A": False, "B": False, "C": False, "D": True, "SC": False, "SW": False, "H": BACK, "V": UP}
        ],
        "neutral_highjump": [
            {"A": False, "B": False, "C": False, "D": True, "SC": False, "SW": False, "H": NEUTRAL, "V": UP}
        ],
        "forward_dash": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
            EMPTY,
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
        ],
        "backward_dash": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL},
            EMPTY,
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL},
        ],
        "forward_fly": [
            {"A": False, "B": False, "C": False, "D": True, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL}
        ],
        "backward_fly": [
            {"A": False, "B": False, "C": False, "D": True, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL}
        ],
        "be1": [
            {"A": False, "B": False, "C": False, "D": True, "SC": False, "SW": False, "H": BACK, "V": DOWN},
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": DOWN},
            {"A": False, "B": False, "C": False, "D": True, "SC": False, "SW": False, "H": BACK, "V": DOWN},
        ],
        "be2": [
            {"A": False, "B": False, "C": False, "D": True, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
            {"A": False, "B": False, "C": False, "D": True, "SC": False, "SW": False, "H": NEUTRAL, "V": DOWN},
        ],
        "be4": [
            {"A": False, "B": False, "C": False, "D": True, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL},
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL},
            {"A": False, "B": False, "C": False, "D": True, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL},
        ],
        "be6": [
            {"A": False, "B": False, "C": False, "D": True, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
            {"A": False, "B": False, "C": False, "D": True, "SC": False, "SW": False, "H": FRONT, "V": NEUTRAL},
        ],
        "switch_to_card_1": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": True, "H": NEUTRAL, "V": NEUTRAL},
        ],
        "switch_to_card_2": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": True, "H": NEUTRAL, "V": NEUTRAL},
            EMPTY,
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": True, "H": NEUTRAL, "V": NEUTRAL},
        ],
        "switch_to_card_3": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": True, "H": NEUTRAL, "V": NEUTRAL},
            EMPTY,
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": True, "H": NEUTRAL, "V": NEUTRAL},
            EMPTY,
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": True, "H": NEUTRAL, "V": NEUTRAL},
        ],
        "switch_to_card_4": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": True, "H": NEUTRAL, "V": NEUTRAL},
            EMPTY,
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": True, "H": NEUTRAL, "V": NEUTRAL},
            EMPTY,
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": True, "H": NEUTRAL, "V": NEUTRAL},
            EMPTY,
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": True, "H": NEUTRAL, "V": NEUTRAL},
        ],
        "use_current_card": [
            {"A": False, "B": False, "C": False, "D": False, "SC": True, "SW": False, "H": NEUTRAL, "V": NEUTRAL}
        ],
        "upgrade_skillcard": [
            {"A": False, "B": False, "C": False, "D": False, "SC": True, "SW": False, "H": BACK, "V": NEUTRAL}
        ]
    }

    def __init__(self, exe_path, actions, max_games=1, start_port=10800, seed=None, device="cpu"):
        self.actions = actions
        super().__init__(exe_path, max_games=max_games, start_port=start_port, seed=seed, device=device)

    def _step(self, tensordict):
        total_reward = 0.0
        tensordict = tensordict[0]
        values = [float(f) for f in tensordict["action"]]
        for input1 in self.actions_buffers[self.actions[values.index(max(values))]]:
            if not self.last_state["done"]:
                assert self.playing
                try:
                    input1["H"] *= self.last_frame["left"].direction
                    input2 = self.EMPTY if self.p2ai is None else self.p2ai.get_inputs(
                        self.last_frame["right"],         self.last_state["left"],
                        self.last_frame["right"].objects, self.last_frame["left"].objects,
                        self.last_state["weathers"]
                    )
                    input2["H"] *= self.last_frame["right"].direction
                    frame = self.instances[0].tick({ "left": input1, "right": input2 })
                    self.last_frame = frame
                    if self.force_health:
                        mod = False
                        if frame["left"].hp > self.force_health[0]:
                            self.last_state["left"]["hp"] = self.force_health[0]
                            mod = True
                        if frame["right"].hp > self.force_health[1]:
                            self.last_state["right"]["hp"] = self.force_health[1]
                            mod = True
                        if mod:
                            self.instances[0].set_health(self.last_state["left"]["hp"], self.last_state["right"]["hp"])
                    new_state = tensor_from_state(self.last_state, frame, self.last_state["params"], tensordict.shape)
                    reward = compute_reward(self.last_state, new_state)
                    new_state["done"] = False
                    total_reward += reward[0]
                    self.last_state = new_state
                except GameEndedException as e:
                    if e.winner == 0:
                        self.last_state["reward"] = DRAW_PENALTY
                    elif e.winner == 1:
                        self.last_state["reward"] = WIN_BONUS
                    else:
                        self.last_state["reward"] = LOSE_PENALTY
                    self.last_state["done"] = True
                    break
                except GameCrashedError:
                    self.last_state["reward"] = CRASH_PENALTY
                    self.last_state["done"] = True
                    self.reconnect()
                    break
        self.last_state["reward"] = total_reward
        return TensorDict(tensorify(self.last_state)).unsqueeze(-1)

    def _make_spec(self):
        super()._make_spec()
        self.action_spec = Composite({ action: Bounded(low=0, high=1, dtype=torch.float32, shape=1) for action in self.actions })
