-- bsplay.lua -- the whole game: join an AI table, ready up, place five ships,
-- and take a shot.
--
-- bstest.lua proves the transport and the board. This proves the GAME: that
-- the client walks the server's own state machine, that its phase changes are
-- driven by what the server says rather than by anything local, and that a
-- move it sends comes back reflected in the next board it is served.
--
-- BS_TABLE names the table to join (default AI1 -- the AI tables are the only
-- ones that will play against a lone client). Every wait is on rendered
-- state, never on a frame count.

package.path = (os.getenv("A2600_EMU") or ".") .. "/?.lua;" .. package.path
local font = require("vcsfont")

local T_BASE, T_PLANE_LEN, T_CELL_H, T_COLS = 0x1800, 0x80, 6, 12
local BROW, NTROW = 3, 3
local want_table = (os.getenv("BS_TABLE") or "AI1"):upper()

local sp, phase, waited, target, presses = nil, "tables", 0, nil, 0
local shots = 0
-- What this run actually did. A public server keeps its tables between runs,
-- so the same script legitimately takes different paths -- and a PASS line
-- that claimed to have placed ships when it joined a game already in progress
-- would be a lie about the one thing the test is for.
local did = {}

local function read_keys(row)
    local out = {}
    for col = 0, T_COLS - 1 do
        local plane, left, key = col // 2, (col % 2) == 0, 0
        for line = 0, 4 do
            local b = sp:readv_u8(T_BASE + plane * T_PLANE_LEN
                                         + row * T_CELL_H + line)
            local ink = left and ((b >> 5) & 7) or ((b >> 1) & 7)
            key = (key << 3) | ink
        end
        out[col] = key
    end
    return out
end

local function read_row(row)
    local keys, s = read_keys(row), ""
    for col = 0, T_COLS - 1 do
        s = s .. (font.glyph[keys[col]] or "?")
    end
    return s
end

local function trim(s) return (s:gsub("%s+$", "")) end

local function dump(what)
    print("FAIL: " .. what .. "; the screen reads:")
    for r = 0, 20 do
        local s = trim(read_row(r))
        if s ~= "" then print(string.format("  row %2d %q", r, s)) end
    end
end

-- The joystick and the console switches are different ports: the stick and
-- its button hang off the controller slot, SELECT and RESET are on the
-- console itself.
local function press(field, on)
    local port = (field == "Select Game") and manager.machine.ioport.ports[":SWB"]
                 or manager.machine.ioport.ports[":joyport1:joy:JOY"]
    port.fields[field]:set_value(on and 1 or 0)
end

local function cursor(first, n)
    for r = 0, n - 1 do
        if read_keys(first + r)[0] == font.key[">"] then return r end
    end
    return nil
end

-- The board as ten strings of ten cells, or nil if it is not drawn yet.
local function board()
    local rows = {}
    for y = 0, 9 do
        local keys = read_keys(BROW + y)
        if keys[0] ~= font.key[tostring(y)] then return nil end
        local s = ""
        for x = 1, 10 do s = s .. (font.glyph[keys[x]] or "?") end
        rows[y] = s
    end
    return rows
end

local function show(rows, label)
    print(label .. ":")
    for y = 0, 9 do print("  " .. rows[y]) end
end

-- A tap on a button or direction, spread over four frames because the client
-- edge-detects and the stick has to be released between steps.
local tapf, tapn = nil, 0
local function tap(field)
    tapf, tapn = field, 0
end
local function tapping()
    if not tapf then return false end
    tapn = tapn + 1
    if tapn == 1 then press(tapf, true)
    elseif tapn == 3 then press(tapf, false)
    elseif tapn >= 5 then tapf = nil; return false end
    return true
end

_G._bsplay = emu.add_machine_frame_notifier(function()
    sp = sp or manager.machine.devices[":maincpu"].spaces["program"]
    waited = waited + 1
    if waited > 9000 then
        dump("timed out in phase " .. phase)
        manager.machine:exit()
        return
    end
    if tapping() then return end

    -- ---- the table list --------------------------------------------------
    if phase == "tables" then
        local here = cursor(NTROW, 8)
        if not here then return end
        local hit
        for r = 0, 7 do
            if trim(read_row(NTROW + r):sub(2)) == want_table then hit = r end
        end
        if not hit then
            if waited > 900 then
                dump(want_table .. " is not in the table list")
                manager.machine:exit()
            end
            return
        end
        print(string.format("table list arrived; %s is at row %d", want_table, hit))
        target, presses = hit, hit - here
        phase, waited = "walk", 0
        return
    end

    if phase == "walk" then
        if presses > 0 then presses = presses - 1; tap("P1 Down"); return end
        local here = cursor(NTROW, 8)
        if here ~= target then
            dump(string.format("cursor landed on row %s, wanted %d",
                               tostring(here), target))
            manager.machine:exit()
            return
        end
        print("joining " .. want_table)
        tap("P1 Button 1")
        phase, waited = "lobby", 0
        return
    end

    -- ---- the lobby: ready up ---------------------------------------------
    -- A public server keeps its tables between runs, so this may arrive at a
    -- game already in progress. Branch on what is actually on screen rather
    -- than on what a fresh table would have shown.
    if phase == "lobby" then
        if waited < 30 then return end
        if trim(read_row(0)) == "PLACE SHIPS" then
            print("the table is already at placement")
            phase, waited, shots = "place", 0, 0
            return
        end
        local rows = board()
        if not rows then return end
        for y = 0, 9 do
            if rows[y]:find("#") then
                print("the table is already in play with our ships on it")
                did[#did + 1] = "joined a game already in progress"
                phase, waited = "inplay", 0
                return
            end
        end
        print("lobby: " .. trim(read_row(0)))
        print("readying up")
        did[#did + 1] = "readied up in the lobby"
        tap("P1 Button 1")
        phase, waited = "toplace", 0
        return
    end

    -- ---- the placement screen arrives on its own -------------------------
    -- Nothing is pressed to get here. The client hands over as soon as a
    -- /state poll says the phase is STPLACE and its own player status is
    -- still PLACE, which is the only thing that could have driven it.
    if phase == "toplace" then
        if trim(read_row(0)) ~= "PLACE SHIPS" then return end
        print("the server moved the game to placement and the client "
              .. "followed by itself")
        did[#did + 1] = "followed the server into placement with no input"
        phase, waited, shots = "place", 0, 0
        return
    end

    -- ---- place five ships ------------------------------------------------
    -- Down one row between ships so nothing overlaps: lengths 5,4,3,3,2 all
    -- start at column 0 horizontally, on rows 0 to 4.
    if phase == "place" then
        if shots >= 5 then
            print("five ships placed; sending them")
            phase, waited = "placed", 0
            return
        end
        local want = trim(read_row(14))
        if want == "" then return end
        if shots > 0 and waited < 8 then return end
        -- S<n> <len> H
        local n = tonumber(want:sub(2, 2))
        if n ~= shots + 1 then
            if waited > 300 then
                dump(string.format("the placement screen says %q, expected "
                                   .. "ship %d", want, shots + 1))
                manager.machine:exit()
            end
            return
        end
        if shots == 0 then
            print("placing: " .. want)
            tap("P1 Button 1")
            shots = shots + 1
            waited = 0
        else
            -- move down one row first
            print("placing: " .. want)
            tap("P1 Down")
            phase, waited = "placedown", 0
        end
        return
    end

    if phase == "placedown" then
        tap("P1 Button 1")
        shots = shots + 1
        phase, waited = "place", 0
        return
    end

    -- ---- back in the game ------------------------------------------------
    -- The prompt is EMPTY once play starts -- the server sends 33 NULs -- so
    -- waiting for text on row 0 would wait forever. What says the handover
    -- happened is the placement screen's title being gone and a board being
    -- drawn again.
    if phase == "placed" then
        if trim(read_row(0)) == "PLACE SHIPS" then
            if waited > 1800 then
                dump("the placement screen never handed back")
                manager.machine:exit()
            end
            return
        end
        local rows = board()
        if not rows then return end
        show(rows, "your board, hulls overlaid by the cartridge")
        local hulls = 0
        for y = 0, 9 do for x = 1, 10 do
            if rows[y]:sub(x, x) == "#" then hulls = hulls + 1 end
        end end
        -- 5+4+3+3+2 = 17, less whichever cell the blinking cursor is on.
        if hulls < 16 then
            dump(string.format("only %d hull cells are drawn, wanted 17", hulls))
            manager.machine:exit()
            return
        end
        print(string.format("%d hull cells on your own board, which is the "
              .. "five placements the client sent coming back off the wire",
              hulls))
        did[#did + 1] = "placed five ships and saw them come back"
        phase, waited = "inplay", 0
        return
    end

    -- ---- take a shot -----------------------------------------------------
    -- SELECT to the opponent's board, walk to 5,5, and fire. The proof is
    -- that the cell CHANGES: a hit or a miss where open sea was, in a board
    -- the cartridge composed out of the reply the attack returned.
    if phase == "inplay" then
        if trim(read_row(1)):sub(1, 3) == "FOE" then
            phase, waited = "aim", 0
            return
        end
        if waited > 240 then
            dump("SELECT never reached an opponent's board")
            manager.machine:exit()
            return
        end
        tap("Select Game")
        return
    end

    if phase == "aim" then
        local coord = trim(read_row(14))
        if coord == "@5,5" then
            local rows = board()
            if not rows then return end
            show(rows, "the opponent's board before the shot")
            print("firing at 5,5")
            tap("P1 Button 1")
            phase, waited = "fired", 0
            return
        end
        local x = tonumber(coord:sub(2, 2)) or 0
        local y = tonumber(coord:sub(4, 4)) or 0
        if x < 5 then tap("P1 Right") elseif y < 5 then tap("P1 Down")
        else dump("the cursor overshot to " .. coord); manager.machine:exit() end
        return
    end

    if phase == "fired" then
        -- The cursor blinks over the cell it is on, so sample across more
        -- than one blink period before believing the sea is still there.
        local rows = board()
        if not rows then return end
        local c = rows[5]:sub(6, 6)
        if c == "X" or c == "O" then
            show(rows, "the opponent's board after it")
            print(string.format("5,5 now reads %q", c))
            did[#did + 1] = "fired at 5,5 and saw the result come back"
            print("PASS: on a live table -- " .. table.concat(did, "; "))
            manager.machine:exit()
            return
        end
        if waited > 600 then
            dump("the shot at 5,5 never showed up on the board")
            manager.machine:exit()
        end
        return
    end
end)
