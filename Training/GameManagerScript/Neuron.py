class Neuron:
    def __init__(self, i=0):
        self.links = []
        self.id = i
        self.value = 0

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

    def get_value(self):
        if len(self.links) == 0:
            return self.value
        val = sum(link[0] * link[1].get_value() for link in self.links) + self.value
        if val < -1:
            return -1
        if val > 1:
            return 1
        return val

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
