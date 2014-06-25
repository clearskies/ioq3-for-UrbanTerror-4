Clearskies' ioquake3 for Urban Terror
=====================================
### Version 4.2.019 ###

This is the official ioquake3 for Urban Terror with my various client and server additions.

**********

Client
------

### Cvars ###
   + `s_soundhax <0|1>` - Disables or enables background and looping sounds. ***Warning: this is frowned upon or prohibited in most, if not all, leagues. Use at your own discretion***
   + `s_debug <0|1>` - Disables or enables output of sounds being played
   + `com_nosplash <0|1>` - Enables or disables the FrozenSand animation
   + `con_nochat <0|1>` - Enables or disables console chat output (when disabled, it will output a newline in the place of a chat message - I don't know why)
   + `s_chatsound <0|1>` - Disables or enables the little tick sound when a line of chat comes in. This is only useful for 4.1 because 4.2 already has this in the QVM.
   + `cl_drawHealth <0|1>` - Disables or enables an on-screen health display (in percent)
   + `r_noBorder <0|1>` - Disables or enables the window decoration (border, buttons, titlebar) - has no effect if `r_fullscreen` is 1
   + `r_jpegQuality <0-100>` - Sets the image quality for screenshots taken with the `screenshotjpeg` command
   + `cl_drawKills <0|1>` - Disables or enables a kill counter on the HUD

### Commands ###
   + `rebind <key> <cvar>` - Binds a key to a cvar and its current value
   + `chatdump <file>` - Dumps chat from the console to a file (chat is basically anything with a colon in it, sorry)
   + `cvar_incr <cvar> <amount>` - Increases a cvar by a specified amount
   + `cvar_decr <cvar> <amount>` - Decreases a cvar by a specified amount
   + `loc` - Displays current location while in game
   + `messagemodec` - Console message mode - enter and execute a command without toggling the console
   + `messagemoder` - Rcon message mode - enter and execute an rcon command without toggling the console

### Other ###
   + <del>Paste support on Linux (Ctrl-V)</del> - This has been integrated into the official client
   + <del>Paste support on Mac (Ctrl-V; Command-V doesn't work)</del> - This has been integrated into the official client
   + <del>Can sort of auto switch between 4.2 and 4.1 assets. This feature is experimental and may cause some interesting game behaviour. To use it, rename your 4.1 q3ut4 folder to q3ut41 and copy the folder into your game directory. When connecting to a 4.1 server, you'll still get an "Invalid game folder" error. Just reconnect, and it should work. Reconnects automatically now!</del> Very messy, so I removed it.

**********

Server
------
### Cvars ###
   + `sv_allowSuicide <0|1>` - Disables or enables suiciding
   + `sv_allowItemdrop <0|1>` - Disables or enables item dropping
   + `sv_allowWeapdrop <0|1>` - Disables or enables weapon dropping
   + `sv_allowTell <0|1>` - Disables or enables private messaging
   + `sv_antiblock <0|1>` - Enables or disables player collisions. *Players must respawn for this to take effect*
   + `sv_allowKnife <0|1>` - Disables or enables usage of the knife. <del>This only works on 4.2 and it will kill the player when they switch to the knife. There may be a delay of around 0.5-3 seconds. No more delay!</del> This makes the knife deal 0 damage. Players will still see the slashing animation, but no damage occurs
   + `sv_forceGear <"">|<gearstring>` - Disables or enables gear forcing. If the value of this cvar is not an empty string, every client's `gear` userinfo setting will be set to this value
   + `sv_fallDamage <0|1>` - Disables or enables fall damage. This is the same as g_nodamage except for the fact that it works when you're not in Jump Mode. *Requires reload*
   + `sv_iceEverywhere <0|1>` - All surfaces become icy. *Requires reload*
   + `sv_specialWater <0|1|2>` - Makes water behave differently. `1` turns water into ice, `2` turns water into lava, and `0` does nothing. *Requires reload*
   + `sv_chatColor <0-9>` - Changes the colour of players' chat messages
   + `sv_allowVote <0|1>` - Completely disables or enables voting

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
