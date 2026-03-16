ModName = "Debug Mode"
ModAuthor = "jerryjhird-public@proton.me"

local enable_logs = true

-- c++ logic no longer prints debug messages so we hook into them and print them ourselves this allows debug messages to be toggleable at runtime --

function on_debug_log(text)
    if enable_logs then
        print("[DEBUG] " .. text) 
    end
end

function on_player_debug_log(id, text)
    if enable_logs then
        print("[DEBUG-P]" .. text)
    end
end