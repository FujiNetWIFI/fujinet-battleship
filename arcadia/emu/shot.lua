-- shot.lua: one snapshot of whatever is on screen, for art and palette
-- work. Pair with DEMO=1 (`make shot`).
local shot = false
local frame = 0
emu.register_frame(function()
    frame = frame + 1
    if not shot and frame >= 120 then
        manager.machine.video:snapshot(); shot = true
    elseif shot and frame >= 150 then
        manager.machine:exit()
    end
end)
