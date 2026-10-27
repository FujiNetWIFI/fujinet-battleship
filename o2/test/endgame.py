#!/usr/bin/env python3
"""endgame.py -- end a game early, for a look at game over (test/scen/over.scen).

    endgame.py [WINNER [TABLE [PLAYER [SERVER]]]]

Waits for the battle at TABLE (ai1) as PLAYER (o2) sees it -- /view, which
changes nothing -- then, 8 seconds in, POSTs the private server's
/debugEndGame/WINNER (the server's seat; 1 by default). The route exists only
when the server isn't in production mode: never point this at the live one.
"""
import sys
import time
import urllib.request

winner = sys.argv[1] if len(sys.argv) > 1 else "1"
table = sys.argv[2] if len(sys.argv) > 2 else "ai1"
player = sys.argv[3] if len(sys.argv) > 3 else "o2"
server = sys.argv[4] if len(sys.argv) > 4 else "http://localhost:8085/"
q = "?table=%s&player=%s&bin=1&v=2" % (table, player)
t0 = time.time()
while time.time() - t0 < 200:
    try:
        b = urllib.request.urlopen(server + "view" + q, timeout=2).read()
        if len(b) > 34 and 10 <= b[34] < 99:
            time.sleep(8)
            req = urllib.request.Request(server + "debugEndGame/" + winner + q, method="POST")
            urllib.request.urlopen(req, timeout=2).read()
            print("ENDGAME posted", flush=True)
            break
    except OSError:
        pass
    time.sleep(1)
