-- resettest.lua -- console RESET mid-session; the sequence must CONTINUE.
--
-- There is no reset line on the Channel F cart connector at all. A console
-- reset reaches the cart only as ROMC 08, and it restarts the CLIENT without
-- restarting the CART -- so a client that numbered its transactions from a
-- counter in its own RAM would replay a sequence the cart has already
-- acknowledged, and read a stale reply as a fresh one. FNGO takes the number
-- from the cart's own persisted ACKSEQ instead.
--
-- Run with FUJINET_DEBUG=1 and read the seq= numbers across the reset line:
-- they must keep climbing, not start again at 1.
-- MAME re-runs the autoboot script on a soft reset, so the Lua state restarts
-- along with the client. The "have we already done this" flag therefore lives
-- in CART RAM, at an address the client never touches -- which is itself part
-- of what is being demonstrated: the cartridge is not reset by the console.
local VSTATE, MARK = 0x8006, 0x8F00
local sp, n, phase, timer = nil, 0, "name", 0
local function tap(port, field, t)
    if t == 1 then manager.machine.ioport.ports[port].fields[field]:set_value(1)
    elseif t == 20 then manager.machine.ioport.ports[port].fields[field]:set_value(0) end
end
_G._rt = emu.add_machine_frame_notifier(function()
    sp = sp or manager.machine.devices[":maincpu"].spaces["program"]
    n = n + 1
    timer = timer + 1
    if phase == "name" then
        -- take the default name so the client gets as far as a real fetch
        if sp:readv_u8(VSTATE) == 1 and timer > 200 then
            tap(":PANEL", "TIME (Button 1)", timer - 200)
            if timer > 240 then phase, timer = "run", 0 end
        elseif sp:readv_u8(VSTATE) >= 2 then
            phase, timer = "run", 0
        elseif sp:readv_u8(VSTATE) ~= 1 then
            timer = 0
        end
    elseif phase == "run" then
        -- let it get past the name screen and fetch something
        if sp:readv_u8(VSTATE) >= 2 and timer > 400 then
            if sp:readv_u8(MARK) == 0xA5 then
                phase, timer = "after", 0
            else
                print("resettest: ---- SOFT RESET ----")
                sp:write_u8(MARK, 0xA5)
                manager.machine:soft_reset()
                phase, timer = "after", 0
            end
        end
        if n > 9000 then
            print("resettest: FAIL -- never got past the name screen")
            manager.machine:exit()
        end
    elseif phase == "after" then
        if sp:readv_u8(VSTATE) >= 1 and timer > 600 then
            print("resettest: client is alive again, state=" .. sp:readv_u8(VSTATE))
            print("resettest: cart RAM marker survived = " ..
                  (sp:readv_u8(MARK) == 0xA5 and "yes" or "NO"))
            print("resettest: check the seq= numbers above -- they must CONTINUE")
            manager.machine.video:snapshot()
            phase, timer = "done", 0
        end
        if timer > 6000 then
            print("resettest: FAIL -- client did not come back")
            manager.machine:exit()
        end
    elseif phase == "done" then
        if timer > 60 then manager.machine:exit() end
    end
end)
