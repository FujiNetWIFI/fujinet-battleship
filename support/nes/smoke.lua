-- smoke.lua -- read the NES screen out of the nametable and give a verdict.
--
-- Modelled on support/coleco/smoke.lua. This client's background tiles are
-- not ASCII: src/nes/mkchr.py bakes a colour pair into every glyph, so a
-- nametable byte is decoded through the tile -> character table the same
-- script writes (support/nes/tilemap.lua). Game art reads as "#".
--
--   FBS_TILEMAP path to support/nes/tilemap.lua (required)
--   FBS_AT      emulated seconds to settle before sampling; measured from the
--               end of FBS_SCRIPT when there is one
--   FBS_SETTLE  emulated seconds before the first press (default 8)
--   FBS_SCRIPT  comma-separated joypad actions to play first, e.g.
--               "a,wait10,a" -- a b select start up down left right, a chord
--               like select+start, and
--               waitN to idle N emulated seconds (default 1)
--   FBS_EXPECT  substring that must appear on screen; sets the exit verdict
--   FBS_QUIET   only print the verdict, not the screen
--   FBS_SNAP    path to also save a PNG screenshot of the sampled frame; a
--               "snap" step in FBS_SCRIPT saves one mid-script, numbered by step

local AT      = tonumber(os.getenv("FBS_AT") or "8")
local EXPECT  = os.getenv("FBS_EXPECT")
if EXPECT == "" then EXPECT = nil end   -- make/env hand through an empty string
local QUIET   = os.getenv("FBS_QUIET")
local TILEMAP = os.getenv("FBS_TILEMAP")
local SCRIPT  = os.getenv("FBS_SCRIPT")

-- The game's 32x24 layout starts on nametable row 3 (src/nes/graphics.c ROW0).
local COLS, ROWS, ROW0 = 32, 24, 3

local tilemap
local function load_tilemap()
    if TILEMAP == nil then return "FBS_TILEMAP is not set" end
    local chunk, err = loadfile(TILEMAP)
    if chunk == nil then return err end
    tilemap = chunk()
    return nil
end

local function read_screen()
    local ppu = manager.machine.devices[":ppu"]
    if ppu == nil then return nil, "no :ppu device" end
    local vram = ppu.spaces["videoram"]
    if vram == nil then return nil, "no videoram space on the PPU" end
    local lines = {}
    for row = 0, ROWS - 1 do
        local chars = {}
        for col = 0, COLS - 1 do
            local t = vram:readv_u8(0x2000 + (row + ROW0) * 32 + col)
            chars[#chars + 1] = tilemap[t] or "?"
        end
        lines[#lines + 1] = table.concat(chars)
    end
    return lines, nil
end

local BTN = { a = "P1 A", b = "P1 B", select = "P1 Select", start = "P1 Start",
              up = "P1 Up", down = "P1 Down", left = "P1 Left", right = "P1 Right" }

-- SETTLE has to outlast the client's start-up round trips -- the appkey reads
-- and the first table fetch. A press delivered while it is still inside one
-- of those is seen by the edge detector and dropped by whatever loop is not
-- yet running.
local HOLD, GAP = 0.30, 0.45
local SETTLE = tonumber(os.getenv("FBS_SETTLE") or "8")

local function is_wait(action)
    return action:match("^wait(%d*)$") ~= nil
end

local function wait_secs(action)
    local n = action:match("^wait(%d*)$")
    return (n == "" or n == nil) and 1 or tonumber(n)
end

-- "select+start" holds both buttons together: a chord.
local function press(action, on)
    for b in action:gmatch("[^+]+") do
        local name = BTN[b]
        if name == nil then error("smoke.lua: unknown action '" .. b .. "'") end
        local f = manager.machine.ioport.ports[":ctrl1:joypad:JOYPAD"].fields[name]
        if f == nil then error("smoke.lua: no joypad field " .. name) end
        if on then f:set_value(1) else f:set_value(0) end
    end
end

local script = {}
if SCRIPT then
    for a in SCRIPT:gmatch("[^,%s]+") do script[#script + 1] = a end
end

if _G.fbs_state == nil then
    _G.fbs_state = { fired = false, step = 1, down = false, t_next = SETTLE }
end
local st = _G.fbs_state

-- The subscription has to stay in a live global: add_machine_frame_notifier
-- hands back an RAII token, and letting it fall out of scope unsubscribes at
-- the next Lua collection.
_G.fbs_sub = emu.add_machine_frame_notifier(function ()
    if st.fired then return end
    local now = manager.machine.time:as_double()

    if st.step <= #script then
        if now < st.t_next then return end
        if script[st.step] == "snap" then
            local base = (os.getenv("FBS_SNAP") or "snap.png"):gsub("%.png$", "")
            local name = base .. "-" .. tostring(st.step) .. ".png"
            manager.machine.screens[":screen"]:snapshot(name)
            emu.print_info("fbs: snap " .. name)
            st.step = st.step + 1
            if st.step > #script then st.t_sample = now + AT end
            return
        end
        if is_wait(script[st.step]) then
            st.t_next = now + wait_secs(script[st.step])
            st.step = st.step + 1
            if st.step > #script then st.t_sample = st.t_next + AT end
            return
        end
        if st.down then
            press(script[st.step], false)
            st.down = false
            st.step = st.step + 1
            st.t_next = now + GAP
            if st.step > #script then st.t_sample = now + AT end
        else
            press(script[st.step], true)
            st.down = true
            st.t_next = now + HOLD
        end
        return
    end

    if now < (st.t_sample or AT) then return end
    st.fired = true

    local err = load_tilemap()
    if err then
        emu.print_info("fbs: FAIL " .. err)
        manager.machine:exit()
        return
    end

    local lines, rerr = read_screen()
    if lines == nil then
        emu.print_info("fbs: FAIL " .. rerr)
        manager.machine:exit()
        return
    end

    if not QUIET then
        print("+--------------------------------+")
        for _, l in ipairs(lines) do print("|" .. l .. "|") end
        print("+--------------------------------+")
    end

    -- "The screen looks right" is not proof on its own: VRAM keeps whatever
    -- was last drawn. The PC and the mailbox status page say whether the
    -- client is still running and what the last transaction said.
    do
        local cpu = manager.machine.devices[":maincpu"]
        local mem = cpu.spaces["program"]
        print(string.format("fbs: PC=%04X ACKSEQ=%02X STATUS=%02X ERR=%02X",
            cpu.state["PC"].value,
            mem:readv_u8(0x5400), mem:readv_u8(0x5401), mem:readv_u8(0x5402)))
    end

    local snap = os.getenv("FBS_SNAP")
    if snap and snap ~= "" then
        local serr = manager.machine.screens[":screen"]:snapshot(snap)
        emu.print_info("fbs: snapshot " .. snap .. (serr and (" FAILED: " .. tostring(serr)) or ""))
    end

    if EXPECT then
        local joined = table.concat(lines, "\n")
        if joined:find(EXPECT, 1, true) then
            print("fbs: PASS")
        else
            print("fbs: FAIL expected " .. string.format("%q", EXPECT))
        end
    end
    manager.machine:exit()
end)
