-- vramrow.lua -- dump a few VRAM rows, to check what actually reached the screen.
local n = 0
_G._vr = emu.add_machine_frame_notifier(function()
    n = n + 1
    if n ~= tonumber(os.getenv("VRAM_FRAME") or "900") then return end
    local m = manager.machine
    for k, v in pairs(m.memory.regions) do print("REGION " .. k .. " size=" .. v.size) end
    local r = m.memory.regions[":vram"]
    if not r then print("no :vram region") manager.machine:exit() return end
    for row = 24, 30 do
        local s = ""
        for col = 20, 40 do s = s .. string.format("%d", r:read_u8(row * 128 + col) & 3) end
        print(string.format("row %2d: %s", row, s))
    end
    manager.machine:exit()
end)
