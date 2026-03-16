## Documentation

the following modules are exposed to every lua script

```cpp
sol::lib::base, 
sol::lib::package, 
sol::lib::table, 
sol::lib::math,
sol::lib::string
```

to start making a mod create a file in the Mods folder and write your mod metadata. for example:
```lua
ModName = "Example Mod"
ModAuthor = "jerryjhird-public@proton.me"
```


Events are functions with specific names that you define in your lua code and when the event associated with that function name happens in game it will call your code
for example if i wanted to do something when the player breaks a block:
```lua
function on_break_block(x, y, z, tileId)
    -- do thing
end
```

exposed functions are functions given to you. for example:
```lua
print("hello world\n")
```

### Events
`on_tick()` this function is called every in game tick

`on_break_block((int)x, (int)y, (int)z,  (int)tileId)` this function is called when a block is broken

`on_stat_awarded((int)statId, (string)statName, (int)difficulty, (int)amountAdded, (int)totalValue)`

`on_player_move((float)x, (float)y, (float)z)`

### Exposed Functions
`log((string)message)` prints text to stdio

`spawn_entity((int)entityId, (float)x, (float)y, (float)z)` spawns an entity/mob