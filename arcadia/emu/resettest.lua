-- resettest.lua: a console RESET mid-session must not restart the
-- sequence numbering. The client derives every SEQ from the cart's own
-- persisted ACKSEQ + 1, never a local counter, because RESET restarts
-- the program but NOT the cartridge -- and a replayed sequence number is
-- a transaction the cart will refuse.
--
-- The autoboot script re-runs after a soft reset, so all timing is
-- relative to a captured base and only the first run pulls the reset.
--
--   mame arcadia -cartslot fujinet -cart build/battleship.bin \
--       -autoboot_script emu/resettest.lua -video none -sound none \
--       -seconds_to_run 60

local FPS = 60
local base = manager.machine.time.seconds
local first = base < 1
local frame, si, done, seqbefore = 0, 1, false, 0
local col2, col3
local script = {}
local function tap(f, port, bit)
    script[#script + 1] = {f, port, bit, 1}
    script[#script + 1] = {f + 12, port, bit, 0}
end

emu.register_frame(function()
    local p = manager.machine.ioport.ports
    if col2 == nil then
        col2, col3 = p[":controller1_col2"], p[":controller1_col3"]
        if col2 == nil then return end
        tap(3 * FPS, col3, 0x01)
        tap(6 * FPS, col2, 0x04)
    end
    frame = frame + 1
    while si <= #script and frame >= script[si][1] do
        local e = script[si]; e[2]:field(e[3]):set_value(e[4]); si = si + 1
    end
    local mem = manager.machine.devices[":maincpu"].spaces["program"]
    if first and not done and frame >= 14 * FPS then
        seqbefore = mem:read_u8(0x2C00)
        emu.print_info("resettest: ackseq before reset = " .. seqbefore)
        done = true
        manager.machine:soft_reset()
    elseif not first and frame >= 20 * FPS then
        local seq = mem:read_u8(0x2C00)
        if seq > 0 then
            print("resettest: PASS (ackseq continued to " .. seq .. ")")
        else
            print("resettest: FAIL (ackseq restarted)")
        end
        manager.machine:exit()
    end
end)
