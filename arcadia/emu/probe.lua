-- probe.lua: drive the client through the whole flow and report.
--
-- Timing is in FRAMES, not seconds: MAME's machine.time.seconds returns
-- the INTEGER second, so a script written in fractional seconds collapses
-- a press and its release into one frame and the key is never seen.
--
--   PROBE_AT="360,900"  frames at which to dump the cell map
--   PROBE_WATCH=1       trace the game-state RAM cells
local FPS = 60
local col1, col2, col3
local function ports()
    local p = manager.machine.ioport.ports
    col1, col2, col3 = p[":controller1_col1"], p[":controller1_col2"], p[":controller1_col3"]
end
local function press(port, bit, on) port:field(bit):set_value(on and 1 or 0) end

local ENTER, FIRE, K5 = 0x01, 0x08, 0x04
local function portof(n) return n == 1 and col1 or (n == 2 and col2 or col3) end
local script = {}
local function tap(frame, portn, bit)
    script[#script + 1] = {frame, function() press(portof(portn), bit, true) end}
    script[#script + 1] = {frame + 12, function() press(portof(portn), bit, false) end}
end
tap(3 * FPS, 3, ENTER)                       -- accept the default name
tap(6 * FPS, 2, K5)                          -- join table 5 (ai1)
-- The ai tables auto-start, so placement comes straight away. Five
-- commits accept the seeded layout; then fire at the enemy board.
for i = 0, 4 do tap((10 + i * 2) * FPS, 2, FIRE) end
-- Walk the reticle between shots so they land on different cells. The
-- disc is the "joysticks" port: 0x01 left, 0x02 right, 0x04 down,
-- 0x08 up for player 1.
local JOY = nil
local function joy(frame, bit)
    script[#script + 1] = {frame, function()
        JOY = JOY or manager.machine.ioport.ports[":joysticks"]
        JOY:field(bit):set_value(1) end}
    script[#script + 1] = {frame + 10, function() JOY:field(bit):set_value(0) end}
end
for i = 0, 25 do
    local f = (24 + i * 3) * FPS
    joy(f - 40, (i % 2 == 0) and 0x02 or 0x04)   -- right, then down
    tap(f, 2, FIRE)
end

local dumps = {}
for s in (os.getenv("PROBE_AT") or ""):gmatch("[^,]+") do dumps[#dumps + 1] = tonumber(s) end
local di, si, frame, last = 1, 1, 0, ""

local function dump(tag)
    local mem = manager.machine.devices[":maincpu"].spaces["program"]
    print("---- " .. tag)
    for row = 0, 25 do
        local base = (row < 13) and (0x1800 + row * 16) or (0x1A00 + (row - 13) * 16)
        local t = ""
        for c = 0, 15 do
            local b = mem:read_u8(base + c) & 0x3F
            if b == 0 then t = t .. " "
            elseif b >= 0x10 and b <= 0x19 then t = t .. string.char(48 + b - 0x10)
            elseif b >= 0x1A and b <= 0x33 then t = t .. string.char(65 + b - 0x1A)
            elseif b == 0x3C then t = t .. "X"      -- hit
            elseif b == 0x3D then t = t .. "o"      -- miss
            elseif b == 0x3E then t = t .. "*"      -- sunk
            elseif b == 0x3F then t = t .. "#"      -- pip
            elseif b >= 0x39 and b <= 0x3B then t = t .. "="   -- hull
            else t = t .. "." end
        end
        print(string.format("r%02d |%s|", row, t))
    end
end

emu.register_frame(function()
    if col1 == nil then ports(); if col1 == nil then return end end
    frame = frame + 1
    while si <= #script and frame >= script[si][1] do
        local e = script[si]
        if type(e[2]) == "function" then e[2]() end
        si = si + 1
    end
    if di <= #dumps and frame >= dumps[di] then dump("f=" .. dumps[di]); di = di + 1 end
    if os.getenv("PROBE_WATCH") then
        local m = manager.machine.devices[":maincpu"].spaces["program"]
        local s = string.format("CURS=%d PEND=%d ST=%d PLST=%d ACT=%d VIEW=%d CUR=%d,%d",
            m:read_u8(0x18EA), m:read_u8(0x1AF7), m:read_u8(0x1AEF), m:read_u8(0x1AF0),
            m:read_u8(0x1AED), m:read_u8(0x1AF1), m:read_u8(0x1AF2), m:read_u8(0x1AF3))
        if s ~= last then print(string.format("f=%d %s", frame, s)); last = s end
    end
end)
