-- smoke.lua -- read the ColecoVision screen out of VRAM and give a verdict.
--
-- Modelled on fujinet-firmware/pico/coleco/emu/screen.lua, but that one reads
-- the pattern NAME table and treats a name-table byte as the character, which
-- only works in Graphics 1 with OS7's ASCII generator. This client runs
-- GRAPHICS II, where the name table is a fixed identity map laid down at mode
-- init and the character is carried by the eight pattern bytes the console
-- writes into the generator for that cell. So the text has to be decoded:
-- every cell's glyph is looked up in the same font.bin the ROM was built with,
-- which turns "what is on screen" back into a string comparison.
--
-- Cell (row, col) lives at generator + row*256 + col*8 -- see
-- z88dk's __tms9918_mode2_printc, which computes exactly that and notes it
-- assumes the generator is at 0, which is where z88dk's mode 2 puts it.
--
--   FBS_FONT    path to src/coleco/font.bin (required)
--   FBS_AT      emulated seconds to settle before sampling; measured from the
--               end of FBS_SCRIPT when there is one
--   FBS_SETTLE  emulated seconds before the first press (default 8)
--   FBS_SCRIPT  comma-separated controller actions to play first, e.g.
--               "fire,wait10,fire" -- fire fire2 up down left right
--               k0..k9 star hash, and waitN to idle N emulated seconds
--               (default 1), for the screens that fetch before they respond
--   FBS_EXPECT  substring that must appear on screen; sets the exit verdict
--   FBS_SNAP    path to also save a PNG screenshot of the sampled frame
--   FBS_QUIET   only print the verdict, not the screen

local AT     = tonumber(os.getenv("FBS_AT") or "8")
local EXPECT = os.getenv("FBS_EXPECT")
if EXPECT == "" then EXPECT = nil end   -- make/env hand through an empty string
local QUIET  = os.getenv("FBS_QUIET")
local FONT   = os.getenv("FBS_FONT")

local SCRIPT = os.getenv("FBS_SCRIPT")

local COLS, ROWS = 32, 24
local FIRST_GLYPH = 0x20        -- font.bin covers 0x20-0x7F, 8 bytes each

-- Pattern -> character, built once from the font the ROM was linked against.
local glyphs = {}

local function load_font()
    if FONT == nil then return "FBS_FONT is not set" end
    local f = io.open(FONT, "rb")
    if f == nil then return "cannot open " .. FONT end
    local data = f:read("*all")
    f:close()
    if #data < 96 * 8 then return FONT .. " is only " .. #data .. " bytes" end
    for i = 0, 95 do
        local key = data:sub(i * 8 + 1, i * 8 + 8)
        -- First definition wins: a font with duplicate glyphs should report the
        -- lower codepoint rather than whichever happened to be scanned last.
        if glyphs[key] == nil then
            glyphs[key] = string.char(FIRST_GLYPH + i)
        end
    end
    return nil
end

local function read_screen()
    local vdp = manager.machine.devices[":tms9928a"]
    if vdp == nil then return nil, "no :tms9928a device" end
    local vram = vdp.spaces["vram"] or vdp.spaces["videoram"] or vdp.spaces["data"]
    if vram == nil then return nil, "no VRAM space on the VDP" end

    local blank = string.rep("\0", 8)
    local lines = {}
    for row = 0, ROWS - 1 do
        local chars = {}
        for col = 0, COLS - 1 do
            local base = row * 256 + col * 8
            local bytes = {}
            for i = 0, 7 do
                bytes[#bytes + 1] = string.char(vram:read_u8(base + i))
            end
            local key = table.concat(bytes)
            -- '?' is a glyph the font does not have: a UDG (card art, the
            -- logo) or a partially drawn cell. Distinct from a real space.
            chars[#chars + 1] = (key == blank) and " " or (glyphs[key] or "?")
        end
        lines[#lines + 1] = table.concat(chars)
    end
    return lines, nil
end

-- coleco.cpp's own ports, not the controller slot device's: the driver carries
-- the inputs and only routes them through the slot.
--
-- The two buttons are NOT both on the joystick port, and MAME's names for them
-- are the reverse of the client's. coleco_joypad() reads the directions and one
-- button with the multiplexer in joystick mode, then switches to keypad mode
-- and reads the other -- so what z88dk calls MOVE_FIRE (and this client treats
-- as select) is MAME's "P1 Button 2", mask 0x4000 on STD_KEYPAD1, while
-- MOVE_FIRE2 (which the client treats as ESC) is MAME's "P1 Button 1", mask
-- 0x40 on STD_JOY1. Note 0x40 on the KEYPAD port is the digit 6, so getting
-- this wrong presses a keypad key and the client sits there looking fine.
local JOY = { up = 0x01, right = 0x02, down = 0x04, left = 0x08,
              fire2 = 0x40 }
local PAD = { k0 = 0x0001, k1 = 0x0002, k2 = 0x0004, k3 = 0x0008,
              k4 = 0x0010, k5 = 0x0020, k6 = 0x0040, k7 = 0x0080,
              k8 = 0x0100, k9 = 0x0200, hash = 0x0400, star = 0x0800 }

-- SETTLE has to outlast the client's start-up round trips -- the lobby appkey
-- reads and the first table fetch. A press delivered while it is still inside
-- one of those is seen by the edge detector and dropped by whatever loop is
-- not yet running.
-- HOLD must stay under the shared input's auto-repeat delay (12 frames =
-- 0.2s): readCommonInput() repeats a held direction after that, so a longer
-- hold moves the cursor twice per scripted press.
local HOLD, GAP = 0.15, 0.45
local SETTLE = tonumber(os.getenv("FBS_SETTLE") or "8")

local function find_port(suffix)
    for tag, port in pairs(manager.machine.ioport.ports) do
        if tag:sub(-#suffix) == suffix then return port end
    end
    error("smoke.lua: no input port ending in " .. suffix)
end

local function is_wait(action)
    return action:match("^wait(%d*)$") ~= nil
end

local function wait_secs(action)
    local n = action:match("^wait(%d*)$")
    return (n == "" or n == nil) and 1 or tonumber(n)
end

local function press(action, on)
    local bit, port
    if action == "fire" then
        bit, port = 0x4000, find_port("STD_KEYPAD1")
    elseif JOY[action] then
        bit, port = JOY[action], find_port("STD_JOY1")
    elseif PAD[action] then
        bit, port = PAD[action], find_port("STD_KEYPAD1")
    else
        error("smoke.lua: unknown action '" .. action .. "'")
    end
    local f = port:field(bit)
    if f == nil then
        error(string.format("smoke.lua: no field %#x on that port", bit))
    end
    -- set_value takes the LOGICAL state: MAME inverts IP_ACTIVE_LOW for you.
    if on then f:set_value(1) else f:clear_value() end
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
-- the next Lua collection, so a callback due several seconds out never fires.
_G.fbs_sub = emu.add_machine_frame_notifier(function ()
    if st.fired then return end

    -- as_double(), not machine.time.seconds: that field is whole seconds, so
    -- comparing against it quantises every interval to a full second and holds
    -- a direction long enough for auto-repeat to walk the cursor several cells
    -- per "press".
    local now = manager.machine.time:as_double()

    if st.step <= #script then
        if now < st.t_next then return end
        if script[st.step] == "snap" then
            -- Mid-script screenshot, numbered by step, next to FBS_SNAP
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
            -- Presses are held for a beat and released: a press that never
            -- lets go is one event to the client's edge detector, not many.
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

    local err = load_font()
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
        emu.print_info("+--------------------------------+")
        for _, l in ipairs(lines) do emu.print_info("|" .. l .. "|") end
        emu.print_info("+--------------------------------+")
    end

    -- "The screen looks right" is not proof on its own: VRAM keeps whatever
    -- was last drawn, so a client that has hung still shows a convincing
    -- screen. The PC and the mailbox status say whether it is still running.
    do
        local cpu = manager.machine.devices[":maincpu"]
        local mem = cpu.spaces["program"]
        emu.print_info(string.format("fbs: PC=%04X ACKSEQ=%02X STATUS=%02X ERR=%02X",
            cpu.state["PC"].value,
            mem:readv_u8(0xFC00), mem:readv_u8(0xFC01), mem:readv_u8(0xFC02)))
    end

    -- FBS_SNAP: also save a real screenshot (colors, tiles) next to the ROM
    local snap = os.getenv("FBS_SNAP")
    if snap and snap ~= "" then
        local serr = manager.machine.screens[":screen"]:snapshot(snap)
        emu.print_info("fbs: snapshot " .. snap .. (serr and (" FAILED: " .. tostring(serr)) or ""))
    end

    if EXPECT then
        local joined = table.concat(lines, "\n")
        if joined:find(EXPECT, 1, true) then
            emu.print_info("fbs: PASS")
        else
            emu.print_info("fbs: FAIL expected " .. string.format("%q", EXPECT))
        end
    end
    manager.machine:exit()
end)
