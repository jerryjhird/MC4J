ModName = "Example Mod"
ModAuthor = "jerryjhird-public@proton.me"

function on_player_move(x, y, z)
    spawn_entity(50, x, y, z) -- creeper
end

function on_tick()
    print("tick\n")
end