-- probe.lua -- watch the client's own state in the arena, once a second.
-- Waiting on program state rather than frame numbers is the rule: this machine
-- has no interrupt source, so nothing about the client is frame-paced.
local sp, n = nil, 0
local function u8(a) return sp:readv_u8(a) end
local function str(a, n)
    local s = ""
    for i = 0, n - 1 do
        local c = u8(a + i)
        if c == 0 then break end
        s = s .. string.char(c)
    end
    return s
end
_G._probe = emu.add_machine_frame_notifier(function()
    sp = sp or manager.machine.devices[":maincpu"].spaces["program"]
    n = n + 1
    if n % 60 ~= 0 then return end
    print(string.format(
        "f%-5d state=%d err=%02X class=%d st=%d plst=%d pc=%d act=%d rx=%d name=%q tbl=%q cur=%d/%d",
        n, u8(0x8006), u8(0x802A), u8(0x8036), u8(0x8031), u8(0x8032),
        u8(0x8030), u8(0x8033), u8(0x8026) + 256 * u8(0x8027),
        str(0x8160, 9), str(0x8170, 9), u8(0x8009), u8(0x8091)))
end)
