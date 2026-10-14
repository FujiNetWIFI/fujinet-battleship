-- smoke.lua: headless end-to-end test. The cart boots straight into the
-- name screen (there is no OS menu on the Arcadia): Enter accepts the
-- ARCADIA default, keypad '5' joins the AI room, FIRE readies up, five
-- more accept the seeded ship layout, then it plays until the check.
--
-- PASS needs a long transaction history AND the server past the opening
-- phases, so a client that joins and then sulks cannot pass.
--
--   FUJINET_DEBUG=1 mame arcadia -cartslot fujinet -cart build/battleship.bin \
--       -autoboot_script emu/smoke.lua -video none -sound none \
--       -seconds_to_run 150 -snapshot_directory build
--
-- Two traps this harness exists to avoid repeating:
--   * MAME's machine.time.seconds is the INTEGER second, so a script
--     written in fractional seconds collapses a press and its release
--     into one frame and the key is never seen. Count frames.
--   * /ready TOGGLES and the server's start countdown is WALL CLOCK, so
--     pressing FIRE repeatedly in the lobby un-readies and cancels it
--     every time. Press once, then wait for the status to move.

local FPS = 60
local SEQ_MIN = tonumber(os.getenv("SMOKE_SEQ") or "35")
local col2, col3, joy, mem
local frame, nextact, held = 0, 0, {}
local readied, placed, shots = false, 0, 0
local verdict, done = nil, false

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

    if not done and frame >= 140 * FPS then
        done = true
        local seq = mem:read_u8(0x2C00)
        local st  = mem:read_u8(0x1AEF)
        if seq >= SEQ_MIN and st >= 10 then
            verdict = string.format("PASS (ackseq %d, status %d, %d shots)", seq, st, shots)
        else
            verdict = string.format("FAIL (ackseq %d, status %d, %d shots)", seq, st, shots)
        end
        emu.print_info("smoke: " .. verdict)
        manager.machine.video:snapshot()
        return
    elseif frame >= 145 * FPS then
        print("smoke: " .. (verdict or "FAIL (never checked)"))
        manager.machine:exit()
        return
    end
    if frame < nextact then return end

    local st  = mem:read_u8(0x1AEF)     -- V_ST: the server's status
    local act = mem:read_u8(0x1AED)     -- V_ACT: 0 = our turn
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
        if not readied then tap(col2, 0x08); readied = true end
        nextact = frame + 60
    elseif st == 1 then
        readied = false
        if placed < 5 then tap(col2, 0x08); placed = placed + 1; nextact = frame + 45
        else nextact = frame + 60 end
    elseif st >= 10 and st ~= 99 and act == 0 then
        if shots % 3 == 0 then tap(joy, 0x02) else tap(joy, 0x04) end
        tap(col2, 0x08); shots = shots + 1; nextact = frame + 60
    else
        nextact = frame + 30
    end
end)
