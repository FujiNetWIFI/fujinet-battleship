-- smoke.lua -- power-on to shots fired, headless, against a LOCAL server.
--
--   cd <servers>/fujinet-game-system/battleship && go run .
--   ENDPOINT=http://127.0.0.1:8080/ ./build.sh && make smoke
--
-- Driven by the CLIENT'S OWN STATE read out of the arena, never by a fixed
-- timeline. Three traps this avoids, all of them already paid for:
--
--   * MAME's machine.time.seconds returns the INTEGER second, so a script
--     written in fractional seconds collapses a press and its release into one
--     frame and the key is never seen. Count frames.
--   * A state variable changes BEFORE the screen it belongs to is drawn, and
--     a BIOS screen clear is most of a second. A press issued the instant the
--     state moves lands and is released before the client's first input scan
--     ever runs, and edge detection cannot see a key that was never held
--     across a scan. So every step waits for the state to be STABLE.
--   * manager.machine.ioport.ports must be re-fetched, not cached.
local VSTATE, VCLASS, VPLST, VACT, VST = 0x8006, 0x8036, 0x8032, 0x8033, 0x8031
local VERR, VRXLO, VCURX, VCURY = 0x802A, 0x8026, 0x8040, 0x8041
local ST_EDIT, ST_TABLE, ST_GAME = 1, 2, 3
local CL_LOBBY, CL_PLACE, CL_PLAY, CL_OVER = 0, 1, 2, 3

local SETTLE = 150              -- frames a state must hold before we act on it
local HOLD, GAP = 20, 30        -- frames a key is held, and the gap after it

local sp
local n, step, queue, shots = 0, 1, {}, 0
local cur, rel, gap, stable, last = nil, 0, 0, 0, -1
local function u8(a) return sp:readv_u8(a) end
local function key(port, field) queue[#queue + 1] = { port, field } end
local function fire(c) for _ = 1, (c or 1) do key(":RIGHT_C", "P1 Push Down") end end
local function down(c) for _ = 1, (c or 1) do key(":RIGHT_C", "P1 Down") end end
-- Walk the reticle to (x,y) from wherever the client actually has it. Reading
-- the client's own cursor rather than counting presses means a refused move --
-- and there are none here, but there could be -- cannot desynchronise us.
local function aim(x, y)
    local cx, cy = u8(VCURX), u8(VCURY)
    for _ = 1, y - cy do key(":RIGHT_C", "P1 Down") end
    for _ = 1, cy - y do key(":RIGHT_C", "P1 Up") end
    for _ = 1, x - cx do key(":RIGHT_C", "P1 Right") end
    for _ = 1, cx - x do key(":RIGHT_C", "P1 Left") end
end
local function field(k) return manager.machine.ioport.ports[k[1]].fields[k[2]] end

_G._smoke = emu.add_machine_frame_notifier(function()
    sp = sp or manager.machine.devices[":maincpu"].spaces["program"]
    n = n + 1

    if cur then
        rel = rel - 1
        if rel <= 0 then field(cur):set_value(0) cur, gap = nil, GAP end
        return
    end
    if gap > 0 then gap = gap - 1 return end
    if #queue > 0 then
        cur = table.remove(queue, 1)
        field(cur):set_value(1)
        rel = HOLD
        return
    end

    local st, cl = u8(VSTATE), u8(VCLASS)
    local now = st * 16 + cl
    if now ~= last then last, stable = now, 0 else stable = stable + 1 end
    if n % 300 == 0 then
        local pc = manager.machine.devices[":maincpu"].state["PC0"]
        print(string.format("  [f%d] step=%d state=%d class=%d plst=%d act=%d st=%d err=%02X rx=%d pc=%04X ship=%d",
            n, step, st, cl, u8(VPLST), u8(VACT), u8(VST), u8(VERR),
            u8(VRXLO) + 256 * u8(VRXLO + 1), pc and pc.value or 0, u8(0x8048)))
    end
    if stable < SETTLE then return end
    if n % 15 ~= 0 then return end

    if step == 1 then
        if st == ST_EDIT then
            print("smoke: keyboard up, taking the default name")
            key(":PANEL", "TIME (Button 1)")
            step = 2
        elseif st == ST_TABLE then
            step = 3                        -- the appkey already had a name
        end
    elseif step == 2 then
        if st == ST_TABLE then step = 3 end
    elseif step == 3 then
        print("smoke: table list, joining AI - 1 on 1")
        down(4)                             -- r2 r1 ai3 ai2 ai1
        fire(1)
        step = 4
    elseif step == 4 then
        if st == ST_GAME then
            print("smoke: seated, class=" .. cl)
            step = 5
        end
    elseif step == 5 then
        if cl == CL_LOBBY then
            print("smoke: readying up")
            fire(1)                         -- /ready/1 carries the value, so a
            step = 6                        --   second press cannot un-ready us
        else
            step = 6
        end
    elseif step == 6 then
        if cl == CL_PLACE and u8(VPLST) == 10 then
            print("smoke: accepting the seeded fleet")
            fire(5)                         -- a seeded layout is always legal
            step = 7
        elseif cl == CL_PLAY or cl == CL_OVER then
            step = 7
        end
    elseif step == 7 then
        if cl == CL_OVER then
            print("smoke: GAME OVER after " .. shots .. " shots")
            manager.machine.video:snapshot()
            step = 9
        elseif cl == CL_PLAY and u8(VACT) == 0 and u8(VPLST) == 0 then
            -- One shot per cell, marching across the board. Firing twice at
            -- the same square is refused by the client on purpose, so a driver
            -- that did not move the reticle would deadlock the game: the turn
            -- never passes and the bot never gets to move.
            aim(shots % 10, math.floor(shots / 10) % 10)
            fire(1)
            shots = shots + 1
            print(string.format("smoke: shot %d at %d,%d (server status %d)",
                shots, (shots - 1) % 10, math.floor((shots - 1) / 10) % 10, u8(VST)))
            if shots >= 60 then
                manager.machine.video:snapshot()
                step = 9
            end
        end
    elseif step == 9 then
        step = 10
        manager.machine:exit()
    end
end)
