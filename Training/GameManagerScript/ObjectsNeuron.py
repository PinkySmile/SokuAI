import random
import math


class ObjectsNeuron:
    obj_to_range = [
        lambda x: x,
        lambda x: x / 1500,
        lambda x: x / 1500,
        lambda x: x / 1500,
        lambda x: x / 1500,
        lambda x: x / 65535,
        lambda x: x / 65535,
    ]

    def __init__(self, i=0):
        self.id = i
        self.objects = []
        self.value = random.random() * 2 - 1
        self.weights = [random.random() * 2 - 1 for _ in range(len(ObjectsNeuron.obj_to_range))]

    def load_from_line(self, line):
        elems = [float(i) for i in line.split(",")]
        self.id = int(elems[0])
        self.weights = elems[1:]

    def resolve_links(self, _):
        pass

    def mutate(self, _):
        for i in range(len(self.weights)):
            # Mutate links
            if random.randint(0, 1) == 0:
                self.weights[i] += math.exp(random.random() / -2) / 4
            else:
                self.weights[i] -= math.exp(random.random() / -2) / 4
            self.weights[i] = max(min(1, self.weights[i]), -1)
        if random.randint(0, 1) == 0:
            self.value += math.exp(random.random() / -2) / 4
        else:
            self.value -= math.exp(random.random() / -2) / 4

    def get_value(self):
        return max(min(sum(
            sum(
                weight * obj for weight, obj in zip(
                    self.weights,
                    (fct(obj) for fct, obj in zip(ObjectsNeuron.obj_to_range, objec))
                )
             ) for objec in self.objects
        ) + self.value, 1), -1)

    def get_dependencies(self):
        return {self.id}

    def copy(self):
        me = ObjectsNeuron(self.id)
        me.weights = self.weights.copy()
        return me

    def save(self, stream):
        stream.write(",".join(
            (str(self.id), ",".join(str(i) for i in self.weights))
        ) + "\n")
