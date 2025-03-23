import random


class DeckFactory:
    cards = [
        [*range(21), *range(100, 112), 200, 201, 204, 206, 207, 208, 209, 210, 214, 219],
        [*range(21), *range(100, 112), 200, 202, 203, 204, 205, 206, 207, 208, 209, 211, 212, 214, 215, 219],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211],
        [*range(21), *range(100, 115), 200, 201, 202, 203, 204, 205, 206, 207, 210, 211, 212, 213],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 208, 212],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 208, 209],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 219],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 208, 215],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 208, 212],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 205, 206, 207, 208, 211, 212],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 211],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 206, 207, 208, 209, 210, 211],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 208, 209],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 210],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 208, 210, 213],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 211],
        [*range(21), *range(100, 112), 200, 201, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214],
        [*range(21), *range(100, 112), 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 212],
        [*range(21), *range(100, 112)]
    ]

    @staticmethod
    def build_deck(character):
        deck = []
        while len(deck) != 20:
            card = random.choice(DeckFactory.cards[character])
            if deck.count(card) >= 4:
                continue
            deck.append(card)
        return deck
