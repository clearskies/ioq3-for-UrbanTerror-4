Clearskies' ioquake3 for Urban Terror
=====================================
### Version 4.2.019 ###

This is the official ioquake3 for Urban Terror with my various client and server additions.

**********

Client
------

### Cvars ###
   + `s_debug <0|1>` - Enables output of sounds being played. Default is `0`
   + `com_nosplash <0|1>` - Disables the FrozenSand animation. Default is `0`
   + `s_chatsound <0|1>` - Enables the little tick sound when a line of chat comes in. This is only useful for 4.1 because 4.2 already has this in the QVM. Default is `1`
   + `cl_drawHealth <0|1>` - Enables health percentage display on the HUD. Default is `0`
   + `r_noBorder <0|1>` - Disables the window decoration (border, buttons, titlebar) - has no effect if `r_fullscreen` is 1. Default is `0`
   + `r_jpegQuality <0-100>` - Sets the image quality for screenshots taken with the `screenshotjpeg` command. Default is `90`
   + `cl_drawKills <0|1>` - Enables a kill counter on the HUD. Default is `0`
   + `cl_teamchatIndicator ""` - Enables a customizable indicatpr to indicate that a chat message is a teamchat. Useful on stupid maps like orbital. Default is `""`
   + `cl_randomRGB <0|1|2|3>` - Generates random armband colours. `1` generates a new armband colour at startup, `2` generates one every map, and `3` generates one every frame. Default is `0`
   + `cl_crosshairHealth <0|1>` - Fades the crosshair colour from red to yellow to green based on the player's health. Default is `0`
   + `clan ""` - Sets an optional clan tag that will be added to your name. Default is `""`
   + `cl_clanPos <0|1>` - Determines the position of the `clan` cvar in your name. `0` will put the clan tag at the beginning, and `1` will put it at the end. Default is `0`
   + `r_drawLegs <0|1>` - Draws your legs. Default is `0`
   + `con_coloredKills <0|1>` - Colours players' names in console kill messages. Default is `0`
   + `con_bgAlpha <0-100>` - Sets the console background opacity. `0` is fully transparent and `100` is fully opaque. Default is `90`
   + `con_coloredHits <0|1|2>` - Colours hit percentages in the console hit log. `1` will colour the damage values and `2` will colour player names as well. Default is `0`
   + `con_prompt ""` - Sets the prompt for the console. Default is `]`
   + `con_height <0-100>` - Sets the console height in percent of the game window's height. Default is `50`
   + `con_bgColor <0-9>` - Sets the console background colour. Values can be any standard Quake 3 colour (0-9). Default is `0`
   + `con_promptColor <0-9>` - Sets the console prompt colour. Values can be any standard Quake 3 colour (0-9). Default is `0`
   + `con_timePrompt <0|1>` - Adds a little time indicator to the console prompt. `1` adds a 24-hour clock and `2` adds a 12-hour clock. Default is `0`
   + `con_scrollLock <0|1>` - Keeps the console in place when a new line comes in. Default is `1`
   + `con_drawScrollbar <0|1>` - Draws a scrollbar in the console to indicate your position in the scrollback. Default is `0`
   + `cl_consoleCommand ""` - Sets the command that will be automatically run when no command prefix (`/` or `\`) is entered. Default is `say`
   + `con_fadeIn <0|1>` - Fades the console in/out instead of sliding it down/up. Default is `0`
   + `con_margin <0-50>` - Adds a margin to the top, left, and right sides of the console. If this is greater than `0`, then the console will become a box. Default is `0`
   + `con_showVersion <0|1>` - Shows the client's version number in the console. Default is `1`
   + `con_tabs <0|1>` - Enables console tabs (see below for description). Default is `0`
   + `con_chatTime <0|1|2>` - Enables a timestamp next to every chat message. `1` adds a 24-hour timestamp and `2` adds a 12-hour timestamp. Default is `0`
   + `con_borderRGB <0-255> <0-255> <0-255> ` - Changes the console border colour. Default is `0 100 100`
   + `cl_deathBind ""` - Executes a command on death. Default is `""`


### Commands ###
   + `rebind <key> <cvar>` - Binds a key to a cvar and its current value
   + `cvar_incr <cvar> <amount>` - Increases a cvar by a specified amount
   + `cvar_decr <cvar> <amount>` - Decreases a cvar by a specified amount
   + `loc` - Displays current location while in game
   + `messagemodec` - Console message mode - enter and execute a command without toggling the console
   + `messagemoder` - Rcon message mode - enter and execute an rcon command without toggling the console
   + `randomRGB` - Randomly generates a new armband colour
   + `maplist` - Outputs the maps that the current server has available
   + `findcvar <string>` - Finds all cvars with with the specified string in their names
   + `cbind <key1> <key2> [action]` - Binds an action to a key combo or outputs the bound action of a key combo
   + `cunbind <key1> <key2>` - Unbinds an action from a key combo
   + `cbindlist` - Lists all combo binds
   + `cunbindall` - Unbinds all combo binds

### Other ###
   + Supports combo binds (binds involving two keys). Use the commands `cbind`, `cunbind`, `cbindlist`, and `cunbindall`. This currently does not work with `+` commands.
   + Console tabs. Tabs separate the console into three areas: `Main`, `Misc`, and `Chat`. `Main` has all the console output, `Chat` has all the chat messages, and `Misc` has everything not in `Chat`. Use `Shift + LEFTARROW` and `Shift + RIGHTARROW` to navigate between tabs.
   + Chat variables:
      + `$hp` - Current health (in percent)
      + `$p` - Current player name
      + `$team` - Current team
      + `$oteam` - Other team
      + `$loc` - Latest location (works even when dead or spec)

**********

Server
------
### Cvars ###
   + `sv_allowSuicide <0|1>` - Enables suicide via `/kill`. Default is `1`
   + `sv_allowItemdrop <0|1>` - Enables item dropping. Default is `1`
   + `sv_allowWeapdrop <0|1>` - Enables weapon dropping. Default is `1`
   + `sv_allowTell <0|1>` - Enables private messaging. Default is `1`
   + `sv_antiblock <0|1>` - Disables player collisions. Default is `0`. *Players must respawn for this to take effect*
   + `sv_removeKnife <0|1>` - Removes the knife. Default is `0`
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
   + `backwards <player>` - Reverses a player's movement

### Player Commands (commands a player can call while connected to the server) ###
   + `ff?` - Informs the player of the status of g_friendlyfire
   + `maplist` - Sends the player a list of all of the maps loaded on the server
   + `mapcycle` - Sends the player the mapcycle
