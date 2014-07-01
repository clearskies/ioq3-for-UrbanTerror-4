Clearskies' ioquake3 for Urban Terror
=====================================
### Version 4.2.019 ###

This is the official ioquake3 for Urban Terror with my various client and server additions.

**********

Client
------

### Cvars ###
   + `s_soundhax <0|1>` - Disables background and looping sounds. Default is `0`. ***Warning: this is frowned upon or prohibited in most, if not all, leagues. Use at your own discretion***
   + `s_debug <0|1>` - Enables output of sounds being played. Default is `0`
   + `com_nosplash <0|1>` - Disables the FrozenSand animation. Default is `0`
   + `con_nochat <0|1|2|3>` - Disables console chat output. `1` will disable public chat, `2` will disable teamchat, and `3` will disable both public and teamchat. Default is `0`
   + `s_chatsound <0|1>` - Enables the little tick sound when a line of chat comes in. This is only useful for 4.1 because 4.2 already has this in the QVM. Default is `1`
   + `cl_drawHealth <0|1>` - Enables health percentage display on the HUD. Default is `0`
   + `r_noBorder <0|1>` - Disables the window decoration (border, buttons, titlebar) - has no effect if `r_fullscreen` is 1. Default is `0`
   + `r_jpegQuality <0-100>` - Sets the image quality for screenshots taken with the `screenshotjpeg` command. Default is `90`
   + `cl_drawKills <0|1>` - Enables a kill counter on the HUD. Default is `0`
   + `cl_teamchatIndicator <0|1>` - Enables a little `(T)` to indicate that a chat message is a teamchat. Useful on stupid maps like orbital. Default is `0`
   + `cl_hpSub <0|1>` - Enables an `$hp` chat variable that gets replaced with your actual health. Default is `0`
   + `cl_randomRGB <0|1|2>` - Generates random armband colours. `1` randomly generates a new armband colour at startup, `2` randomly generates one when connecting to a server. Default is `0`
   + `cl_playerSub <0|1>` - Enables an `$p` chat variable that gets replaced with the current player name. If you're playing, it's your name, if you're spectating, it's the name of the player you're spectating. Default is `0`
   + `cl_weapAutoSwitch <0|1|2>` - Automatically switches weapons when ammo runs out. `1` will make it switch to the previous weapon, and `2` will make it switch to the next weapon. Default is `0`
   + `cl_weapAutoReload <0|1|>` - Automatically reloads when ammo runs out. `cl_weapAutoSwitch` will take precedence over this. Default is `0`
   + `cl_crosshairHealth <0|1>` - Fades the crosshair colour from red to yellow to green based on the player's health. Default is `0`
   + `clan` - Sets an optional clan tag that will be added to your name. Default is `""`
   + `cl_clanPos <0|1>` - Determines the position of the `clan` cvar in your name. `0` will put the clan tag at the beginning, and `1` will put it at the end. Default is `0`
   + `r_drawLegs <0|1>` - Draws your legs. Default is `0`

### Commands ###
   + `rebind <key> <cvar>` - Binds a key to a cvar and its current value
   + `chatdump <file>` - Dumps chat from the console to a file (chat is basically anything with a colon in it, sorry)
   + `cvar_incr <cvar> <amount>` - Increases a cvar by a specified amount
   + `cvar_decr <cvar> <amount>` - Decreases a cvar by a specified amount
   + `loc` - Displays current location while in game
   + `messagemodec` - Console message mode - enter and execute a command without toggling the console
   + `messagemoder` - Rcon message mode - enter and execute an rcon command without toggling the console
   + `randomRGB` - Randomly generates a new armband colour

### Other ###
   + <del>Paste support on Linux (Ctrl-V)</del> - This has been integrated into the official client
   + <del>Paste support on Mac (Ctrl-V; Command-V doesn't work)</del> - This has been integrated into the official client
   + <del>Can sort of auto switch between 4.2 and 4.1 assets. This feature is experimental and may cause some interesting game behaviour. To use it, rename your 4.1 q3ut4 folder to q3ut41 and copy the folder into your game directory. When connecting to a 4.1 server, you'll still get an "Invalid game folder" error. Just reconnect, and it should work. Reconnects automatically now!</del> Very messy, so I removed it.

**********

Server
------
### Cvars ###
   + `sv_allowSuicide <0|1>` - Enables suicide via `/kill`. Default is `1`
   + `sv_allowItemdrop <0|1>` - Enables item dropping. Default is `1`
   + `sv_allowWeapdrop <0|1>` - Enables weapon dropping. Default is `1`
   + `sv_allowTell <0|1>` - Enables private messaging. Default is `1`
   + `sv_antiblock <0|1>` - Disables player collisions. Default is `0`. *Players must respawn for this to take effect*
   + `sv_allowKnife <0|1>` - Enables usage of the knife. Default is `1`. <del>This only works on 4.2 and it will kill the player when they switch to the knife. There may be a delay of around 0.5-3 seconds. No more delay!</del> This makes the knife deal 0 damage. Players will still see the slashing animation, but no damage occurs
   + `sv_forceGear <"">|<gearstring>` - Enables gear forcing. If the value of this cvar is not an empty string, every client's `gear` userinfo setting will be set to this value. Default is `""`
   + `sv_fallDamage <0|1>` - Enables fall damage. This is the same as g_nodamage except for the fact that it works when you're not in Jump Mode. Default is `1`. *Requires reload*
   + `sv_iceEverywhere <0|1>` - Makes all surfaces act like ice. Default is `0`. *Requires reload*
   + `sv_specialWater <0|1|2>` - Makes water behave differently. `1` turns water into ice, `2` turns water into lava, and `0` does nothing. Default is `0`. *Requires reload*
   + `sv_chatColor <0-9>` - Changes the colour of players' chat messages. Default is `3`
   + `sv_allowVote <0|1>` - Enables voting. Default is `1`
   + `sv_infiniteStamina <0|1>` - Enables unlimited stamina in any gamemode. Default is `0`
   + `sv_noRecoil <0|1>` - Disables recoil and movement inaccuracy. Default is `0`
   + `sv_infiniteAmmo <0|1>` - Enables unlimited ammo. In burst mode, weapons will only consume 2 bullets. Default is `0`
   + `sv_infiniteWalljumps <0|1>` - Enables unlimited walljumps. Default is `0`
   + `sv_weaponCycle <0|1>` - Removes weapon shooting delay. Default is `0`

### Server Commands (rcon / stdin only) ###
   + `invisible <player>` - Toggles player invisibility
   + `setscore <player> <value>` - Sets a player's score
   + `setdeaths <player> <value>` - Sets a player's deaths
   + `invulnerable <player>` - Makes a player invulnerable until they die
   + `freeze <player>` - Prevents a player from moving
   + `teleport <player> <toPlayer>` - Teleports `player` to `toPlayer`
   + `callvoteas <player> <vote type> [vote value]` - Calls a vote as a certain player. Useful in conjunction with `sv_allowVote 0` so that votes can be called through rcon but not by the players themselves

### Player Commands (commands a player can call while connected to the server) ###
   + `ff?` - Informs the player of the status of g_friendlyfire
   + `maplist` - Sends the player a list of all of the maps loaded on the server
   + `mapcycle` - Sends the player the mapcycle
