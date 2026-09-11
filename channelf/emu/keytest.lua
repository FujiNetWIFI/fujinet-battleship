-- keytest.lua -- does a keypress reach the client at all?
--
-- The first thing to reach for when a driver script seems to be ignored: it
-- walks the on-screen keyboard with the stick and then tries every console
-- button, printing the client's own cursor after each. If this passes and a
-- driver still does nothing, the driver is pressing too early -- see the
-- settle note in smoke.lua.
local VSTATE, VEDX, VEDY, VEDLEN = 0x8006, 0x81A0, 0x81A1, 0x81A2
local seq = {"P1 Right","TIME (Button 1)","MODE (Button 3)","HOLD (Button 2)","START (Button 4)"}
local sp, n, timer, i, phase = nil, 0, 0, 1, "wait"
local function port(f) return f:match("^P1") and ":RIGHT_C" or ":PANEL" end
_G._kt = emu.add_machine_frame_notifier(function()
    n = n + 1
    sp = sp or manager.machine.devices[":maincpu"].spaces["program"]
    timer = timer + 1
    if phase == "wait" then
        if sp:readv_u8(VSTATE) == 1 and timer > 120 then
            print("keytest: editor up") phase, timer = "keys", 0
        elseif sp:readv_u8(VSTATE) ~= 1 then timer = 0 end
        if n > 3000 then print("keytest: FAIL -- editor never came up") manager.machine:exit() end
    elseif phase == "keys" then
        if i > #seq then phase, timer = "check", 0
        elseif timer == 1 then manager.machine.ioport.ports[port(seq[i])].fields[seq[i]]:set_value(1)
        elseif timer == 10 then manager.machine.ioport.ports[port(seq[i])].fields[seq[i]]:set_value(0)
        elseif timer >= 40 then
            print(string.format("  after %-18s state=%d edx=%d edy=%d len=%d",
                seq[i], sp:readv_u8(VSTATE), sp:readv_u8(VEDX), sp:readv_u8(VEDY),
                sp:readv_u8(VEDLEN)))
            i = i + 1 timer = 0
        end
    elseif phase == "check" then
        if timer > 60 then manager.machine.video:snapshot() phase = "done" timer = 0 end
    elseif phase == "done" then
        if timer > 30 then manager.machine:exit() end
    end
end)
