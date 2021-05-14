import random

from Training import DeckFactory


class BaseAI:
    DOWN = 1
    UP = -1
    BACK = -1
    FRONT = 1
    NEUTRAL = 0
    EMPTY = {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": NEUTRAL, "V": NEUTRAL}
    buffered_input = []
    actions = [
        "1a",
        "2a",
        "3a",
        "4a",
        "5a",
        "6a",
        "8a",
        "66a",
        "2b",
        "3b",
        "4b",
        "5b",
        "6b",
        "66b",
        "1c",
        "2c",
        "3c",
        "4c",
        "5c",
        "6c",
        "66c",
        "236b",
        "236c",
        "623b",
        "623c",
        "214b",
        "214c",
        "421b",
        "421c",
        "22b",
        "22c",
        "nothing",
        "forward",
        "backward",
        "block",
        "forward_jump",
        "backward_jump",
        "neutral_jump",
        "forward_highjump",
        "backward_highjump",
        "neutral_highjump",
        "forward_dash",
        "backward_dash",
        "forward_fly",
        "backward_fly",
        "be1",
        "be2",
        "be4",
        "be6",
        "switch_to_card_1",
        "switch_to_card_2",
        "switch_to_card_3",
        "switch_to_card_4",
        "use_current_card"
    ]
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
            {"A": True, "B": False, "C": True, "D": False, "SC": False, "SW": False, "H": BACK, "V": DOWN}
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
        "block": [
            {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": BACK, "V": NEUTRAL}
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
        ]
    }

    def __init__(self, chr, palette):
        self.chr = chr
        self.palette = palette
        self.deck_factory = DeckFactory.DeckFactory()

    def on_win(self):
        pass

    def on_lose(self):
        pass

    def on_timeout(self):
        self.on_lose()

    def on_game_start(self, my_chr, opponent_chr, input_delay):
        pass

    def can_play_as(self, char_id):
        return True

    def can_play_against(self, char_id):
        return True

    def get_prefered_character(self):
        return {
            "character": self.chr,
            "palette": self.palette,
            "deck": self.deck_factory.build_deck(self.chr)
        }

    @staticmethod
    def get_action(me, opponent, my_projectiles, opponent_projectiles, weather_infos):
        return random.choice(BaseAI.actions)

    def get_inputs(self, me, opponent, my_projectiles, opponent_projectiles, weather_infos):
        if not len(self.buffered_input):
            self.buffered_input = BaseAI.actions_buffers[
                self.get_action(me, opponent, my_projectiles, opponent_projectiles, weather_infos)
            ].copy()
        return self.buffered_input.pop(0)
