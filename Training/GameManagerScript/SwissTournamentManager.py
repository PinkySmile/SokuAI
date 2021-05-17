import GameManager
import math
import threading
import traceback


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
        left_points = sum((result[0] == 1) * 100000 + (result[1][0] - result[2][0]) * 1000 + (result[1][1] - result[2][1]) for result in results)
        right_points = sum((result[0] == 2) * 100000 + (result[2][0] - result[1][0]) * 1000 + (result[2][1] - result[1][1]) for result in results)
        return (left_points != right_points) + (left_points < right_points)

    @staticmethod
    def update_match_ais(match, winner):
        print("Match {} vs {} {} is {}".format(match[0][0], match[1][0], "result" if winner == 0 else "winner", ["a draw", match[0][0], match[1][0]][winner]))
        match[0][1] <<= 1
        match[1][1] <<= 1
        match[0][3].append(True)
        match[1][3].append(False)
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
        match = None
        try:
            while True:
                match = self.matches.pop(0)
                print("Playing match {} vs {}".format(match[0][0], match[1][0]))
                self.game.left_ai = match[0][0]
                self.game.right_ai = match[1][0]
                self.update_match_ais(match, self.get_match_winner(
                    self.game.run(
                        self.game.left_ai.get_prefered_character(),
                        self.game.right_ai.get_prefered_character(),
                        self.stage,
                        self.music,
                        self.nb,
                        self.frame_timout,
                        self.input_delay
                    )
                ))
        except IndexError:
            pass
        except ConnectionResetError:
            traceback.print_exc()
            print("Our number of allies grows thin !")
            # TODO: Do we try to play it again on another game instance ?
            self.update_match_ais(match, -1)


class SwissTournamentManager:
    def __init__(self, game_pool, port_start, game_path, input_delay=0, time_limit=float("inf"), first_to=3, ini_path=None):
        print("Opening", game_pool, "games")
        self.game_managers = [GameManager.GameManager(game_path, port_start + i, (None, None), tps=6000, has_display=False, has_sound=False, ini_path=ini_path) for i in range(game_pool)]
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
            print("Pool {} contains {}".format(pool[-1][2], pool))
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

    def play_tournament(self):
        # AI, win score, score, sides (True is left)
        all_ais = [[ai, 0, 0, []] for ai in self.ais]
        nb = math.ceil(math.log(len(self.ais), 2))
        print("Starting tournament...")
        print("There are {} players so {} rounds".format(len(self.ais), nb))
        for i in range(nb):
            self.play_matches(self.make_matches(all_ais))
        return all_ais
