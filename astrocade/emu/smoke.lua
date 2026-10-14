-- smoke.lua: headless smoke test of the whole flow, against a LOCAL
-- battleship server (its table order is fixed: createTable prepends and
-- the hidden "test" table is filtered out, so the list reads High Seas,
-- Cape Fuji, AI-3, AI-2, AI-1 -- "AI - 1 on 1" is digit 5).
-- Press keypad 1 at the OS menu to launch, pull the trigger to accept the
-- default name, keypad 6 to join the 1-bot table, trigger to ready up,
-- then five spaced triggers confirm the seeded ships (seeds are always
-- legal, so blind confirms work), and periodic triggers after that fire
-- at the resumed cursor whenever the turn comes around. Snapshot near
-- the end.
--   mame ... -autoboot_script emu/smoke.lua -video none -sound none \
--        -seconds_to_run 100
local function port_by_suffix(suffix)
    for tag, port in pairs(manager.machine.ioport.ports) do
        if tag:sub(-#suffix) == suffix then return port end
    end
    return nil
end

local function tap(name, field)
    local p = port_by_suffix(name)
    if p then p:field(field):set_value(1) end
end
local function untap(name, field)
    local p = port_by_suffix(name)
    if p then p:field(field):clear_value() end
end

local phase = 0
local shot = false
local trig = false
emu.register_frame(function()
    local t = manager.machine.time.seconds
    if phase == 0 and t >= 3 then
        emu.print_info("smoke.lua: keypad 1 (launch)")
        tap("KEYPAD3", 0x10)
        phase = 1
    elseif phase == 1 and t >= 5 then
        untap("KEYPAD3", 0x10)
        phase = 2
    elseif phase == 2 and t >= 8 then
        emu.print_info("smoke.lua: trigger (accept name)")
        tap("ctrl1:joy:HANDLE", 0x10)
        phase = 3
    elseif phase == 3 and t >= 9 then
        untap("ctrl1:joy:HANDLE", 0x10)
        phase = 4
    elseif phase == 4 and t >= 13 then
        emu.print_info("smoke.lua: keypad 5 (join AI 1-on-1)")
        tap("KEYPAD2", 0x08)
        phase = 5
    elseif phase == 5 and t >= 14 then
        untap("KEYPAD2", 0x08)
        phase = 6
    elseif phase == 6 and t >= 18 then
        emu.print_info("smoke.lua: trigger (ready)")
        tap("ctrl1:joy:HANDLE", 0x10)
        phase = 7
    elseif phase == 7 and t >= 19 then
        untap("ctrl1:joy:HANDLE", 0x10)
        phase = 8
    elseif phase == 8 and t >= 26 then
        -- placement + play: a trigger every ~4 s confirms the five seeded
        -- ships, then keeps firing at the resumed cursor each turn
        local m = t % 4
        if m < 0.5 and not trig then
            tap("ctrl1:joy:HANDLE", 0x10)
            trig = true
        elseif m >= 0.5 and trig then
            untap("ctrl1:joy:HANDLE", 0x10)
            trig = false
        end
    end
    if not shot and t >= 94 then
        emu.print_info("smoke.lua: snapshot")
        manager.machine.video:snapshot()
        shot = true
    elseif t >= 96 then
        manager.machine:exit()
    end
end)
