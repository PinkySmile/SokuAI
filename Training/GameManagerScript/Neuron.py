import random
import math


class Neuron:
    link_break_chance = (1, 1000)
    link_create_chance = (4, 1000)

    def __init__(self, i=0):
        self.links = []
        self.id = i
        self.value = random.random() * 2 - 1

    def load_from_line(self, line):
        self.links = []
        elems = [float(i) for i in line.split(",")]
        self.id = int(elems[0])
        self.value = int(elems[1])
        for i in range(int(elems[2])):
            self.links.append([elems[3 + i * 2], elems[4 + i * 2]])

    def resolve_links(self, elems):
        for k, link in enumerate(self.links):
            if isinstance(link[1], int):
                link[1], = (i for i in elems if i.id == link[1])

    def mutate(self, others):
        to_remove = []
        for i, link in enumerate(self.links):
            # Mutate links
            if random.randint(0, 1) == 0:
                link[0] += math.exp(random.random() / -2) / 4
            else:
                link[0] -= math.exp(random.random() / -2) / 4
            link[0] = max(min(1, link[0]), -1)
            if len(self.links) != 0 and random.randrange(0, self.link_break_chance[1]) < self.link_break_chance[0]:
                to_remove.insert(0, i)
        if random.randint(0, 1) == 0:
            self.value += math.exp(random.random() / -2) / 4
        else:
            self.value -= math.exp(random.random() / -2) / 4
        for i in to_remove:
            self.links.pop(i)
        # Add some links
        while random.randrange(0, self.link_create_chance[1]) < self.link_create_chance[0]:
            self.links.append([random.random() * 2 - 1, random.choice(tuple(filter(lambda x: self.id not in x.get_dependencies(), others)))])

    def get_value(self):
        if len(self.links) == 0:
            return self.value
        val = sum(link[0] * link[1].get_value() for link in self.links) + self.value
        return min(max(val, -1), 1)

    def get_dependencies(self):
        return set(j for i in ((link[1].id, *link[1].get_dependencies()) for link in self.links) for j in i)

    def copy(self):
        me = Neuron(self.id)
        me.value = self.value
        me.links = [[link[0], link[1].id] for link in self.links]
        return me

    def save(self, stream):
        stream.write(",".join(
            (str(self.id), str(len(self.links)), str(self.value), *(",".join((str(link[0]), str(link[1].id))) for link in self.links))
        ) + "\n")
