-- resettest.lua: console-RESET continuity. Launch, join, soft-reset the
-- console mid-session, relaunch and rejoin; the FUJINET_DEBUG transaction
-- log must show the sequence numbers CONTINUING across the reset (the
-- client derives SEQ from the cart's persisted ACKSEQ, never a local
-- counter). The autoboot script re-runs after a soft reset, so all
-- timing is relative to this run's start, and only the first run pulls
-- the reset.
--   FUJINET_DEBUG=1 mame ... -autoboot_script emu/resettest.lua \
--       -video none -sound none -seconds_to_run 40
local function pbs(s)
    for tag, port in pairs(manager.machine.ioport.ports) do
        if tag:sub(-#s) == s then return port end
    end
end
local base = manager.machine.time.seconds
local first = base < 1
local phase = 0
emu.register_frame(function()
    local t = manager.machine.time.seconds - base
    if phase == 0 and t >= 3 then pbs("KEYPAD3"):field(0x10):set_value(1); phase = 1
    elseif phase == 1 and t >= 3.5 then pbs("KEYPAD3"):field(0x10):clear_value(); phase = 2
    elseif phase == 2 and t >= 5 then pbs("ctrl1:joy:HANDLE"):field(0x10):set_value(1); phase = 3
    elseif phase == 3 and t >= 5.5 then pbs("ctrl1:joy:HANDLE"):field(0x10):clear_value(); phase = 4
    elseif phase == 4 and t >= 8 then pbs("ctrl1:joy:HANDLE"):field(0x10):set_value(1); phase = 5
    elseif phase == 5 and t >= 8.5 then pbs("ctrl1:joy:HANDLE"):field(0x10):clear_value(); phase = 6
    elseif phase == 6 and t >= 15 then
        if first then
            emu.print_info("resettest: SOFT RESET")
            manager.machine:soft_reset()
            phase = 99          -- this callback is done; the re-run takes over
        else
            phase = 7
        end
    elseif phase == 7 and t >= 17 then
        emu.print_info("resettest: final snapshot")
        manager.machine.video:snapshot()
        phase = 8
    elseif phase == 8 then manager.machine:exit() end
end)
