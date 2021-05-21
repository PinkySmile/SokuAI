import GameManager
import math
import threading
import traceback
import pprint


display = False
# display = True
sound = None
# sound = (10, 10)
tps = 60000
pp = pprint.PrettyPrinter(indent=4)


class GameThread(threading.Thread):
    def __init__(self, matches, game, stage=0, music=0, nb=1, frame_timout=float("inf"), input_delay=0):
        super().__init__(None, None)
        self.game = game
        self.stage = stage
        self.music = music
        self.nb = nb
        self.frame_timout = frame_timout
        self.input_delay = input_delay
        self.matches = matches

    @staticmethod
    def get_match_winner(results):
        left_points = sum((result[0] == 1) * 1000000 + (result[1][0] - result[2][0]) * 20000 + (result[1][1] - result[2][1]) for result in results)
        right_points = sum((result[0] == 2) * 1000000 + (result[2][0] - result[1][0]) * 20000 + (result[2][1] - result[1][1]) for result in results)
        return (left_points != right_points) + (left_points < right_points)

    @staticmethod
    def update_match_ais(match, winner, side):
        print("Match {} vs {} {} is {}".format(match[0][0], match[1][0], "result" if winner == 0 else "winner", ["a draw", match[0][0], match[1][0]][winner]))
        match[0][1] <<= 1
        match[1][1] <<= 1
        match[0][3].append(side is True)
        match[1][3].append(side is False)
        if winner == 0:
            match[0][2] += 0.5
            match[1][2] += 0.5
            return
        if winner == 1:
            match[0][2] += 1
            match[0][1] |= 1
            return
        if winner == 2:
            match[1][2] += 1
            match[1][1] |= 1

    def run(self):
        while True:
            try:
                match = self.matches.pop(0)
            except IndexError:
                return
            side = sum(match[0][3]) <= sum(match[1][3])
            self.game.left_ai = match[0 if side else 1][0]
            self.game.right_ai = match[1 if side else 0][0]
            print("Playing match {} vs {}".format(self.game.left_ai, self.game.right_ai))
            try:
                self.update_match_ais(match, self.get_match_winner(
                    self.game.run(
                        self.game.left_ai.get_prefered_character(),
                        self.game.right_ai.get_prefered_character(),
                        self.stage,
                        self.music,
                        self.nb,
                        self.frame_timout,
                        self.input_delay,
                        True
                    )
                ), side)
            except ConnectionResetError:
                traceback.print_exc()
                print("Match was aborted")
                self.update_match_ais(match, -1, -1)
                return


class GameOpenThread(threading.Thread):
    def __init__(self, container, *args):
        super().__init__(None, None)
        self.container = container
        self.args = args

    def run(self):
        self.container.append(GameManager.GameManager(*self.args))


class SwissTournamentManager:
    def __init__(self, game_pool, port_start, game_path, input_delay=0, time_limit=float("inf"), first_to=2, ini_path=None):
        print("Opening", game_pool, "games")
        self.game_managers = []
        threads = []
        for i in range(game_pool):
            thr = GameOpenThread(self.game_managers, "{}/{}/th123.exe".format(game_path, i), port_start + i, (None, None), tps, display, sound, ini_path)
            thr.start()
            threads.append(thr)
        for thread in threads:
            thread.join()
        self.ais = []
        self.first_to = first_to
        self.input_delay = input_delay
        self.timeout = time_limit

    @staticmethod
    def make_matches(ais):
        print("Creating match list")
        pools = {}
        result = []
        for i in ais:
            pools.setdefault(i[2], []).append(i)
        pools = sorted(pools.values(), key=lambda x: x[0][2])
        leftover = None
        for pool in pools:
            if leftover is not None:
                pool.insert(0, leftover)
            leftover = None
            print("Pool {} contains {} ais".format(pool[-1][2], len(pool)))
            # TODO: Try to uniform the sides each AI played and match AIs with the same number of wins in a row together
            # TODO: Don't match AIs together twice
            result += zip(pool[:len(pool)//2], pool[len(pool)//2:] if len(pool) % 2 == 0 else pool[len(pool)//2:-1])
            if len(pool) % 2 != 0:
                leftover = pool[-1]
        if leftover is not None:
            leftover[1] <<= 1
            leftover[1] |= 1
            leftover[2] += 1
            leftover[3].append(False)
        return result

    def play_matches(self, matches):
        threads = [GameThread(matches, game, 0, 0, self.first_to * 2 - 1, self.timeout, self.input_delay) for game in self.game_managers]
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()

    def play_tournament(self, nb_rounds):
        # AI, win score, score, sides (True is left)
        all_ais = [[ai, 0, 0, []] for ai in self.ais]
        print("Starting tournament...")
        print("There are {} players and {} rounds".format(len(self.ais), nb_rounds))
        for i in range(nb_rounds):
            print(f"Round {i} start !")
            self.play_matches(self.make_matches(all_ais))
        return all_ais
