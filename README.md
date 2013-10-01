Clearskies' ioquake3 for Urban Terror
=====================================

This is the official ioquake3 for Urban Terror with my various client and server additions.

**********

Client
------

### Cvars ###
   + `s_soundhax <0|1>` - Disables or enables background and looping sounds. ***Warning: this is frowned upon or prohibited in at least most leagues. Use at your own discretion***
   + `s_debug <0|1>` - Disables or enables output of sounds being played
   + `com_nosplash <0|1>` - Enables or disables the FrozenSand animation
   + `con_nochat <0|1>` - Enables or disables console chat output (when disabled, it will output a newline in the place of a chat message - I don't know why)

### Commands ###
   + `rebind <key> <cvar>` - Binds a key to a cvar and its current value
   + `chatdump <file>` - Dumps chat from the console to a file (chat is basically anything with a colon in it, sorry)
   + `cvar_incr <cvar> <amount>` - Increases a cvar by a specified amount
   + `cvar_decr <cvar> <amount>` - Decreases a cvar by a specified amount

### Other ###
   + <del>Paste support on Linux (Ctrl-V)</del> - This has been integrated into the official client
   + <del>Paste support on Mac (Ctrl-V; Command-V doesn't work)</del> - This has been integrated into the official client
   + Can sort of auto switch between 4.2 and 4.1 assets. This feature is experimental and may cause some interesting game behaviour. To use it, rename your 4.1 q3ut4 folder to q3ut41 and copy the folder into your game directory. When connecting to a 4.1 server, you'll still get an "Invalid game folder" error. Just reconnect, and it should work.
   + I made it not send a bunch of userinfo that really isn't necessary

**********

Server
------
### Cvars ###
   + `sv_allowSuicide <0|1>` - Disables or enables suiciding
   + `sv_allowItemdrop <0|1>` - Disables or enables item dropping
   + `sv_allowWeapdrop <0|1>` - Disables or enables weapon dropping
   + `sv_allowTell <0|1>` - Disables or enables private messaging
   + `sv_antiblock <0|1>` - Enables or disables player collisions

### Commands ###
   + None

### Other ###
   + None
