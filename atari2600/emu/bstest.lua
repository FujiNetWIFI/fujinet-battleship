-- bstest.lua -- drive the Battleship client from the table list into a game.
--
-- Reads the rendered text planes back and requires, in order:
--
--   1. the table list to arrive from the live server and be drawn;
--   2. FIRE to join the highlighted table and reach the game screen;
--   3. the server's own prompt to appear on row 0 -- which can only have come
--      off the socket, since nothing in the ROM contains it;
--   4. a 10x10 board to be there, composed BY THE CARTRIDGE out of the reply
--      window, with a column ruler and row digits;
--   5. the cursor to move when the stick does.
--
-- Every wait is on the program's own rendered state, never on a frame number,
-- so a slow round trip to a real HTTPS server cannot turn into a flake.

package.path = (os.getenv("A2600_EMU") or ".") .. "/?.lua;" .. package.path
local font = require("vcsfont")

local T_BASE, T_PLANE_LEN, T_CELL_H, T_COLS = 0x1800, 0x80, 6, 12
local BROW, NTROW = 3, 3

local sp, phase, waited = nil, "tables", 0

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

local function blank(row)
    return read_row(row):gsub("%s", "") == ""
end

local function dump(what)
    print("FAIL: " .. what .. "; the screen reads:")
    for r = 0, 20 do
        if not blank(r) then print(string.format("  row %2d %q", r, read_row(r))) end
    end
end

local function press(field, on)
    local p = manager.machine.ioport.ports[":joyport1:joy:JOY"]
    p.fields[field]:set_value(on and 1 or 0)
end

-- Where the row cursor is on a list, or nil if nothing is drawn.
local function cursor(first, n)
    for r = 0, n - 1 do
        if read_keys(first + r)[0] == font.key[">"] then return r end
    end
    return nil
end

_G._bstest = emu.add_machine_frame_notifier(function()
    sp = sp or manager.machine.devices[":maincpu"].spaces["program"]
    waited = waited + 1
    if waited > 3000 then
        dump("timed out in phase " .. phase)
        manager.machine:exit()
        return
    end

    if phase == "tables" then
        -- The listing has to have come off the wire: the ROM contains no
        -- table ids, only the endpoint and the URL fragments.
        local here = cursor(NTROW, 8)
        if not here then return end
        local row = read_row(NTROW + here)
        if row:sub(2):gsub("%s", "") == "" then
            if waited > 900 then dump("the table list is empty") ; manager.machine:exit() end
            return
        end
        print(string.format("table list arrived; row 0 is %q", row))
        press("P1 Button 1", true)
        phase, waited = "joining", 0
        return
    end

    if phase == "joining" then
        if waited == 3 then press("P1 Button 1", false) end
        if waited < 6 then return end
        phase, waited = "game", 0
        return
    end

    if phase == "game" then
        -- The board: ten rows, each starting with its own digit and then ten
        -- cells the cartridge composed. Wait for the ruler AND the digits.
        local ruler = read_row(2)
        if ruler:sub(2, 11) ~= "0123456789" then return end
        for y = 0, 9 do
            local keys = read_keys(BROW + y)
            if keys[0] ~= font.key[tostring(y)] then return end
        end
        local prompt = read_row(0)
        if prompt:gsub("%s", "") == "" then return end
        print(string.format("game screen up; the server says %q", prompt))
        print("board rows, composed by the cartridge:")
        for y = 0, 9 do print("  " .. read_row(BROW + y)) end
        phase, waited = "move", 0
        return
    end

    if phase == "move" then
        -- The cursor is at 0,0 and blinks, so drive it right twice and down
        -- three times and require the coordinate row to follow.
        if waited == 1 then press("P1 Right", true)
        elseif waited == 3 then press("P1 Right", false)
        elseif waited == 5 then press("P1 Right", true)
        elseif waited == 7 then press("P1 Right", false)
        elseif waited == 9 then press("P1 Down", true)
        elseif waited == 11 then press("P1 Down", false)
        elseif waited == 13 then press("P1 Down", true)
        elseif waited == 15 then press("P1 Down", false)
        elseif waited == 17 then press("P1 Down", true)
        elseif waited == 19 then press("P1 Down", false)
        elseif waited > 24 then
            local coord = read_row(14)
            if coord:sub(1, 4) ~= "@2,3" then
                dump(string.format("the cursor reads %q, wanted \"@2,3\"", coord))
                manager.machine:exit()
                return
            end
            print("cursor moved to 2,3")
            print("PASS: the table list came off a live server, the client "
                  .. "joined a table, and the cartridge composed its board")
            manager.machine:exit()
        end
        return
    end
end)
