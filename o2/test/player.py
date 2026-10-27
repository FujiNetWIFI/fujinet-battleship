#!/usr/bin/env python3
"""player.py -- a second human for the rig: plays Battleship over HTTP.

    player.py [--server URL] [--table ID] [--name NAME] [--delay S]
              [--sleep-turns N] [--leave-after N] [--games N]

Joins TABLE as NAME, readies in the lobby, places a fixed fleet (five ships
across rows 0-4 at x=0), and on its turn fires at the first cell some live
opponent hasn't marked, DELAY seconds into the turn. --sleep-turns N lets
its first N turns time out (the server skips them); --leave-after N leaves
the table after N shots. Reads the binary v=2 state, the same bytes the O2
does (src/equ.inc). Prints one line per change of status or turn.
"""
import argparse
import sys
import time
import urllib.request

SEAT_LOBBY, SEAT_PLAY = 10, 115


def get(server, path, table, name):
    url = "%s%s?table=%s&player=%s&bin=1&v=2" % (server, path, table, name)
    with urllib.request.urlopen(url, timeout=5) as r:
        return r.read()


def parse(b):
    n, status, pstat, active, movet = b[0], b[34], b[35], b[36], b[37]
    prompt = b[1:34].split(b"\0")[0].decode()
    seats = []
    if status == 0:
        for k in range(n):
            o = 59 + SEAT_LOBBY * k
            seats.append({"name": b[o:o + 9].split(b"\0")[0].decode(), "status": b[o + 9]})
    elif len(b) >= 49 + SEAT_PLAY * n:
        for k in range(n):
            o = 49 + SEAT_PLAY * k
            seats.append({"name": b[o:o + 9].split(b"\0")[0].decode(), "status": b[o + 9],
                          "field": b[o + 10:o + 110], "left": sum(b[o + 110:o + 115])})
    return {"n": n, "status": status, "pstat": pstat, "active": active,
            "movet": movet, "prompt": prompt, "seats": seats, "last": b[38] if len(b) > 38 else 0}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--server", default="http://localhost:8085/")
    ap.add_argument("--table", default="r1")
    ap.add_argument("--name", default="py")
    ap.add_argument("--delay", type=float, default=1.0)
    ap.add_argument("--sleep-turns", type=int, default=0)
    ap.add_argument("--leave-after", type=int, default=-1)
    ap.add_argument("--games", type=int, default=1)
    a = ap.parse_args()
    shots, slept, games, seen, turn_at = 0, 0, 0, None, None
    while True:
        s = parse(get(a.server, "state", a.table, a.name))
        key = (s["status"], s["active"], s["pstat"])
        if key != seen:
            print("PLAYER status %d pstat %d active %d movet %d seats %s %r" % (
                s["status"], s["pstat"], s["active"], s["movet"],
                ",".join("%s:%d" % (x["name"], x.get("left", x["status"])) for x in s["seats"]),
                s["prompt"]), flush=True)
            seen, turn_at = key, time.time()
        if s["status"] == 0 and s["pstat"] != 3 and s["n"] > 1:
            get(a.server, "ready/1", a.table, a.name)
        elif s["status"] == 1 and s["pstat"] == 10:
            get(a.server, "place/0,10,20,30,40", a.table, a.name)
        elif s["status"] in (10, 11, 12, 13) and s["active"] == 0 and s["pstat"] == 0:
            if slept < a.sleep_turns:
                if time.time() - turn_at > s["movet"] + 2:
                    slept += 1
            elif time.time() - turn_at >= a.delay:
                live = [x for x in s["seats"][1:] if x["status"] != 1]
                cell = next(p for p in range(100) if any(x["field"][p] == 0 for x in live))
                get(a.server, "attack/%d" % cell, a.table, a.name)
                shots += 1
                print("PLAYER fires %d" % cell, flush=True)
                if shots == a.leave_after:
                    get(a.server, "leave", a.table, a.name)
                    print("PLAYER left", flush=True)
                    return
        elif s["status"] == 99 and seen and key[0] == 99 and turn_at and time.time() - turn_at > 3:
            games += 1
            turn_at = None
            if games >= a.games:
                print("PLAYER done", flush=True)
                return
        time.sleep(0.4)


if __name__ == "__main__":
    sys.exit(main())
