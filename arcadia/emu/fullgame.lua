-- fullgame.lua: play until the server declares the game over, then dump
-- the game-over screen. Walks the reticle in a raster so every ship is
-- eventually found. Timing is in frames (machine.time.seconds is integer).
local FPS = 60
local col2, col3, joy, mem
local frame, placed = 0, 0
local held, nextact, cell = {}, 0, 0
local reported = false
local overat = 0
local readied = false

local function tap(port, bit)
    port:field(bit):set_value(1)
    held[#held + 1] = {port, bit, frame + 10}
end

emu.register_frame(function()
    local p = manager.machine.ioport.ports
    if col2 == nil then
        col2, col3, joy = p[":controller1_col2"], p[":controller1_col3"], p[":joysticks"]
        mem = manager.machine.devices[":maincpu"].spaces["program"]
        if col2 == nil then return end
    end
    frame = frame + 1
    for i = #held, 1, -1 do
        if frame >= held[i][3] then held[i][1]:field(held[i][2]):set_value(0); table.remove(held, i) end
    end
    if frame < nextact then return end

    local st  = mem:read_u8(0x1AEF)     -- V_ST
    local act = mem:read_u8(0x1AED)     -- V_ACT
    local cx, cy = mem:read_u8(0x1AF2), mem:read_u8(0x1AF3)

    if mem:read_u8(0x1AF6) == 0 then
        -- V_CNT is 0 until the table list lands, so this covers the name
        -- screen AND a /tables fetch that failed while fujinet-pc settled
        -- (the client then waits on a key to retry). Enter serves both.
        if frame >= 3 * FPS then tap(col3, 0x01); nextact = frame + 120 end
    elseif mem:read_u8(0x1ADA) == 0 then
        -- Not in a table yet. The /tables fetch can fail while fujinet-pc
        -- settles after a previous client, and the client then waits on a
        -- key press to retry -- so keep offering one until TBLBUF (the
        -- joined table id) is actually set.
        tap(col2, 0x04); nextact = frame + 180
    elseif st == 0 then
        -- The ready-up lobby. /ready TOGGLES, and the server's start
        -- countdown is WALL CLOCK, so a script that keeps pressing FIRE
        -- here un-readies and cancels the countdown every time. Press
        -- once, then stay quiet until the status actually moves.
        if not readied then tap(col2, 0x08); readied = true end
        nextact = frame + 60
    elseif st == 1 and placed < 5 then
        readied = false
        tap(col2, 0x08); placed = placed + 1; nextact = frame + 45
    elseif st >= 10 and st ~= 99 and act == 0 then
        -- raster: aim at cell `cell`, one disc step at a time, then fire
        local tx, ty = cell % 10, math.floor(cell / 10)
        if cx < tx then tap(joy, 0x02); nextact = frame + 8
        elseif cx > tx then tap(joy, 0x01); nextact = frame + 8
        elseif cy < ty then tap(joy, 0x04); nextact = frame + 8
        elseif cy > ty then tap(joy, 0x08); nextact = frame + 8
        else tap(col2, 0x08); cell = (cell + 7) % 100; nextact = frame + 45 end
    elseif st == 99 and not reported then
        -- Let the client actually repaint first: this fires the instant
        -- the status byte lands, which is BEFORE RENDGO has run, so an
        -- immediate dump shows the previous frame.
        if overat == 0 then overat = frame; nextact = frame + 150; return end
        if frame < overat + 150 then return end
        reported = true
        print(string.format("fullgame: PASS game over, winner index %d", act))
        manager.machine.video:snapshot()
        for row = 0, 25 do
            local base = (row < 13) and (0x1800 + row*16) or (0x1A00 + (row-13)*16)
            local t = ""
            for c = 0, 15 do
                local b = mem:read_u8(base + c) & 0x3F
                if b == 0 then t = t .. " "
                elseif b >= 0x10 and b <= 0x19 then t = t .. string.char(48+b-0x10)
                elseif b >= 0x1A and b <= 0x33 then t = t .. string.char(65+b-0x1A)
                elseif b == 0x3C then t = t .. "X" elseif b == 0x3D then t = t .. "o"
                elseif b == 0x3E then t = t .. "*" elseif b == 0x3F then t = t .. "#"
                elseif b >= 0x39 and b <= 0x3B then t = t .. "="
                else t = t .. "." end
            end
            print(string.format("r%02d |%s|", row, t))
        end
        manager.machine:exit()
    end
end)
