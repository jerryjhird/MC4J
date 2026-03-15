### Creating a mod
to make a mod simply create a lua file in the {project_root}/Mods folder and it should appear in the "Mods" title screen and pause menu
if you are running the binary directly please specify the Mods in ENV like so:
```bash
MODS=../../Mods ./Minecraft.Client
```

### Documentation
hooks are functions you setup and when the event associated with that hook happens in game it will call your code
for example if i wanted to do something when the player breaks a block:
```lua
function on_break_block(x, y, z, tileId)
    -- do thing
end
```

exposed functions are functions given to you. for example:
```lua
log("hello world")
```

### Hooks
`on_break_block(x, y, z, tileId)` this function is called when a block is broken

`on_tick()` this function is called every in game tick


### Exposed Functions
`log(message)` prints text to stdio

`spawn_entity(int entityId, float x, float y, float z)` spawns an entity/mob