import random
import inspect
import os
import math
import Neuron
import ObjectsNeuron
import BaseAI

chr_names = [
    "Reimu",
    "Marisa",
    "Sakuya",
    "Alice",
    "Patchouli",
    "Youmu",
    "Remilia",
    "Yuyuko",
    "Yukari",
    "Suika",
    "Reisen",
    "Aya",
    "Komachi",
    "Iku",
    "Tenshi",
    "Sanae",
    "Cirno",
    "Meiling",
    "Utsuho",
    "Suwako"
]
state_neurons_count = 38 + 3 + 14
input_neurons_count = state_neurons_count * 2 + 3 + 2
objects_offset = 0
weather_offset = 3
state_offset = 5
hand_index = 22
skills_index = 24

new_neuron_chance = (1, 100)

weather_weight = [
    lambda x: x / 19 if x <= 19 else -1, # displayed
    lambda x: x / 19 if x <= 19 else -1, # active
    lambda x: x / 999,                   # timer
]
weight_table = [
    lambda x: x,                          # direction
    lambda x: x / 1240,                   # opponentRelativePos.x
    lambda x: x / 1240,                   # opponentRelativePos.y
    lambda x: x / 1240,                   # distToLeftCorner
    lambda x: x / 1240,                   # distToRightCorner
    lambda x: x / 1240,                   # distToGround
    lambda x: x / 1000,                   # action
    lambda x: x / 50,                     # actionBlockId
    lambda x: x / 50,                     # animationCounter
    lambda x: x / 50,                     # animationSubFrame
    lambda x: x / 65535,                  # frameCount
    lambda x: x / 10000,                  # comboDamage
    lambda x: x / 200,                    # comboLimit
    lambda x: x,                          # airBorne
    lambda x: x / 10000,                  # hp
    lambda x: x / 3,                      # airDashCount
    lambda x: x / 10000,                  # spirit
    lambda x: x / 10000,                  # maxSpirit
    lambda x: x / 200,                    # untech
    lambda x: x / 250,                    # healingCharm
    lambda x: x / 1200,                   # swordOfRapture
    lambda x: x / 2,                      # score
    lambda x: x / 220,                    # hand
    lambda x: x / 220,                    # hand
    lambda x: x / 220,                    # hand
    lambda x: x / 220,                    # hand
    lambda x: x / 220,                    # hand
    lambda x: x / 500,                    # cardGauge
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: -1 if x == 255 else x / 4,  # skills
    lambda x: x / 4,                      # fanLevel
    lambda x: x / 1200,                   # dropInvulTimeLeft
    lambda x: x if x < 0 else x / 1500,   # superArmorTimeLeft
    lambda x: x if x < 0 else x / 3000,   # superArmorHp
    lambda x: x / 600,                    # milleniumVampireTimeLeft
    lambda x: x / 1200,                   # philosoferStoneTimeLeft
    lambda x: x / 300,                    # sakuyasWorldTimeLeft
    lambda x: x / 300,                    # privateSquareTimeLeft
    lambda x: x / 600,                    # orreriesTimeLeft
    lambda x: x / 480,                    # mppTimeLeft
    lambda x: x / 1500,                   # kanakoCooldown
    lambda x: x / 1500,                   # suwakoCooldown
    lambda x: x / 70,                     # objectCount
]


class NeuronAI(BaseAI.BaseAI):
    path = os.path.dirname(inspect.getfile(inspect.currentframe())) + "/generated/"

    @staticmethod
    def get_latest_gen(my_char_id, op_char_id):
        gen = -1
        try:
            for i in os.listdir(NeuronAI.path + "{} vs {}".format(chr_names[my_char_id], chr_names[op_char_id])):
                try:
                    gen = max(gen, int(i))
                except ValueError:
                    pass
        except FileNotFoundError:
            pass
        return gen

    def create_required_path(self, my_char_id, op_char_id):
        try:
            os.makedirs(NeuronAI.path + "{} vs {}/{}".format(chr_names[my_char_id], chr_names[op_char_id], 0 if self.generation < 0 else self.generation))
        except FileExistsError:
            pass

    def __init__(self, palette, index, generation):
        super().__init__(6, palette)
        self.input_delay = 0
        self.index = index
        self.generation = generation
        self.neurons = []
        self.character = 0

    def init(self, my_chr, opponent_chr):
        self.character = my_chr
        if self.generation < 0:
            self.generation = 0
            self.create_base_neurons()
            output = Neuron.Neuron(input_neurons_count)
            output.value = random.random() * 2 - 1
            for neuron in self.neurons:
                output.links.append([random.random() * 2 - 1, neuron])
            self.neurons.append(output)
            self.save(my_chr, opponent_chr)
            return
        self.load_file(my_chr, opponent_chr)

    def save(self, my_chr, opponent_chr):
        with open(NeuronAI.path + "{} vs {}/{}/{}.neur".format(chr_names[my_chr], chr_names[opponent_chr], self.generation, self.index), "w") as fd:
            fd.write(str(self.palette) + "\n")
            for neuron in self.neurons[input_neurons_count:]:
                neuron.save(fd)

    def on_game_start(self, my_chr, opponent_chr, input_delay):
        self.input_delay = input_delay

    def create_base_neurons(self):
        self.neurons = [
            ObjectsNeuron.ObjectsNeuron(0),
            ObjectsNeuron.ObjectsNeuron(1)
        ]
        for i in range(2, input_neurons_count):  # I'd like to import GameInstance for this though
            self.neurons.append(Neuron.Neuron(i))

    def load_file(self, my_chr, opponent_chr):
        self.create_base_neurons()
        with open(NeuronAI.path + "{} vs {}/{}/{}.neur".format(chr_names[my_chr], chr_names[opponent_chr], self.generation, self.index)) as fd:
            lines = fd.read().split("\n")
            self.palette = int(lines[0])
            for line in lines[1:]:
                neuron = Neuron.Neuron(len(self.neurons))
                neuron.load_from_line(line)
                self.neurons.append(neuron)
        for neuron in self.neurons:
            neuron.resolve_links(self.neurons)
        self.neurons.sort(key=lambda a: a.id)
        for i, v in enumerate(self.neurons):
            if i != v.id:
                raise Exception("Invalid file. Neuron #{} have index {}".format(i, v.id))

    def mate_once(self, other, i, my_chr, opponent_chr):
        child = NeuronAI(random.choice((self.palette, other.palette)), i, self.generation + 1)
        child.create_base_neurons()

        # Copy either parent1's or parent2's neuron
        for i in range(input_neurons_count, max(len(self.neurons), len(other.neurons))):
            if random.randint(0, 1) == 0 and i < len(self.neurons) or i >= len(other.neurons):
                child.neurons.append(self.neurons[i].copy())
            else:
                child.neurons.append(other.neurons[i].copy())
        for neuron in child.neurons:
            neuron.resolve_links(child.neurons)

        # Mutate neurons
        for neuron in child.neurons:
            neuron.mutate(child.neurons)

        # Add some neurons
        while random.randrange(0, new_neuron_chance[1]) < new_neuron_chance[0]:
            neur = Neuron.Neuron(len(child.neurons))
            neur.links = [[random.random() * 2 - 1, random.choice(child.neurons)]]
            while neur.links[0][1].id == input_neurons_count:
                neur.links[0][1] = random.choice(child.neurons)
            for neuron in child.neurons:
                if neuron.id != input_neurons_count and random.randint(0, 10) == 0:
                    neur.links.append([random.random() * 2 - 1, neuron])
                elif neuron.id >= input_neurons_count and neuron.id not in neur.get_dependencies() and random.randint(0, 10) == 0:
                    neuron.links.append([random.random() * 2 - 1, neur])
            neur.value = random.random() * 2 - 1
            child.neurons.append(neur)
        # And finally we save all this mess
        child.create_required_path(my_chr, opponent_chr)
        child.save(my_chr, opponent_chr)
        return child

    def mate(self, other, my_chr, opponent_chr, start_id, nb):
        return [self.mate_once(other, i, my_chr, opponent_chr) for i in range(start_id, start_id + nb)]

    def can_play_matchup(self, my_char_id, op_char_id):
        return os.path.exists(self.path + "{} vs {}".format(chr_names[my_char_id], chr_names[op_char_id]))

    def get_action(self, me, opponent, my_projectiles, opponent_projectiles, weather_infos):
        rr = me[:hand_index] + tuple(me[hand_index]) + me[hand_index+1:skills_index] + tuple(me[skills_index]) + me[skills_index+1:]
        self.neurons[  objects_offset  ].objects = my_projectiles
        self.neurons[objects_offset + 1].objects = opponent_projectiles
        for i, k in enumerate(weather_infos):
            self.neurons[weather_offset + i].value = weather_weight[i](k)
        for i, v in enumerate(rr):
            self.neurons[i + state_offset].value = weight_table[i](v)
        for i, v in enumerate(opponent[:hand_index] + tuple(opponent[hand_index]) + opponent[hand_index+1:skills_index] + tuple(opponent[skills_index]) + opponent[skills_index+1:]):
            self.neurons[i + state_offset + state_neurons_count].value = weight_table[i](v)
        value = self.neurons[input_neurons_count].get_value()
        result = int(round((value + 1) * len(BaseAI.BaseAI.actions) / 2 - 0.5))
        if result == len(BaseAI.BaseAI.actions):
            real = BaseAI.BaseAI.actions[-1]
        else:
            real = BaseAI.BaseAI.actions[result]
        return real

    def __str__(self):
        return "NAI {} gen{}-{}".format(chr_names[self.character], self.generation, self.index)
