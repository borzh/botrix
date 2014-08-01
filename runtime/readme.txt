Botrix is a plugin for Half-Life 2: Deathmatch that allows to play with bots.


Changelog
=========

v0.0.2
* Default base folder location is <MOD DIRECTORY>/addons/botrix.
* Searching for base folder in 4 different locations.
* Log with levels (none, trace, debug, info, warning, error).
* Added command "version".
* Don't repeat bot names.
* Fixed several crashes.
 

Fast installation for HL2DM
===========================
Copy folders "addons" to your mod dir, i.e.
    "<SteamDir>/SteamApps/common/Half-Life 2 Deathmatch/hl2mp/"

Launch Half-Life 2 Deathmatch with -insecure parameter (this is not required for
dedicated servers). Create new game, map dm_underpass. Type "botrix bot add"
several times at the console to create bots.

Have fun!


Custom installation for HL2DM
=============================
As you may know Steam differs notions of GAME FOLDER (where hl2.exe is) and MOD
FOLDER (where gameinfo.txt is).

Plugin searches for folder named "botrix" in next locations:
 - MOD FOLDER/addons (default location, visible only for one mod)
 - MOD FOLDER
 - GAME FOLDER
 - BASE FOLDER (best location, visible for several mods)

Inside folder "botrix" plugin stores configuration file and waypoints. It is
better to have "botrix" folder inside BASE FOLDER to share it between games.
 
Example of SINGLE-PLAYER GAME:
  Base folder (where all games are):
    C:/Steam/SteamApps/common
  Game folder:
    C:/Steam/SteamApps/common/Half-Life 2 Deathmatch/
  Mod folder:
    C:/Steam/SteamApps/common/Half-Life 2 Deathmatch/hl2mp/ <- (addons and cfg here)
  Botrix folder:
    C:/Steam/SteamApps/common/ <- (botrix here)

Example of DEDICATED SERVER:
  Base folder (where all dedicated servers are):
    C:/DedicatedServer/
  Game folder:
    C:/DedicatedServer/HL2DM/
  Mod folder:
    C:/DedicatedServer/HL2DM/hl2mp/ <- (addons and cfg here)
  Botrix folder:
    C:/DedicatedServer/ <- (botrix here)

This plugin is UNSIGNED, so you need to execute hl2.exe with -insecure parameter
in single-player game. You can also set it by Steam, just right-click on the
game and select it's properties / set launch parameters or whatever.
Note that dedicated server doesn't need that.

For now waypoints were created for 2 maps: dm_underpass and dm_steamlab.
If you want to create waypoints use botrix commands to create and save them.
After doing that, send me your files, and I will make sure to test and publish
them on my site.

Any ideas are also appreciated, and if time will allow me, I could implement
them in future versions.

Please, feel free to send feedback to: botrix.plugin@gmail.com


Configuration for HL2DM
=======================
Inside botrix folder there is a file: config.ini. Make sure that both GAME
FOLDER and MOD FOLDER appear in section [HalfLife2Deathmatch.mod], under "games"
and "mods" keys.

For given example of SINGLE-PLAYER GAME you should have:
    games = Half-Life 2 Deathmatch
    mods = hl2mp

For given example of DEDICATED SERVER you should have:
    games = hl2dm
    mods = hl2mp


Admins
======
User of single player game has full admin rights.
For DEDICATED SERVER there is a section in config.ini: [User access]. You
should read comments for that section to know how to add admins for Botrix.


Other mods
==========
Botrix should work with other mods, say Counter-Strike: Source. Check out
*.items.* and *.weapons sections in config.ini to figure out how items and 
weapons are handled.

If you could make bots work for other mods, please send me your config file and
I will publish it on my site.


Limitations = future work
=========================
- Bots can't use manual weapons, grenades.
- During team play bots can stuck with each other.
- Bots can stuck with non breakable objects.
- Bots can't chat.
- Remove useless commands for dedicated server.


Console commands
================
Every command in Botrix starts with "botrix".
At any time you can check for help of the command typing incomplete command (for
example just type botrix at the console to see available commands).
Note that some commands will not work on dedicated server (because text/line
drawing is a debug drawing, that can be done only on server side, it's not
transmitted to client side).

Waypoints commands
------------------
Waypoint commands begin with a word "waypoint". For example if the command to
save waypoints in a file is "save", then complete command that user has to
introduce should be: "botrix waypoint save".

    - addtype param: add type to waypoint. Param can be one of: 'stop', 'camper',
      'sniper', 'weapon', 'ammo', 'health', 'armor', 'health_machine',
      'armor_machine', 'button', 'see_button'.
    - area. Set of area commands. Can be one of:
        * remove param: remove area with name param.
        * rename param1 param2: change area name from param1 to param2.
        * set param: set the area of current waypoint to param.
        * show: print area names at the console.
    - argument param: set waypoint arguments (angles, ammo count, weapon index,
      armor count, health count).
    - autocreate param: automatically create new waypoints. Param can be 'off' -
      disable, or 'on' - enable. Waypoint will be added when player goes far
      away from the current one. By default it is off.
    - clear: delete all waypoints.
    - create: create new waypoint at current player's position.
    - destination: set given (when param is specified) or current waypoint as
      destination waypoint.
    - drawtype param: defines how to draw waypoint. Param can be 'none', 'all',
      'next' or mix of: 'line', 'beam', 'box', 'text'.
    - info: display info of current waypoint at the console.
    - load: load waypoints from file.
    - move: moves given (when param is specified) or destination waypoint to
      current player's position.
    - remove: delete given (when param is specified) or current waypoint.
    - removetype: remove all waypoint types.
    - reset: reset current waypoint to the nearest one.
    - save: save waypoints into a file.

Waypoint path commands
----------------------
Waypoint path defines a connection between 2 waypoints. Current path is a path
from current waypoint (nearest one, use 'botrix waypoint reset' command to
reset) to waypoint destination (use 'botrix waypoint destination' command to
set). Waypoint path commands start with a word "path".

    - autocreate param: enable auto path creation for new waypoints. Param can
      be 'off' - disable, or 'on' - enable). If disabled, only path from
      'destination' waypoint to a new one will be added.
    - addtype param: add type to current path. Can be mix of: 'crouch', 'jump',
      'break', 'sprint', 'ladder', 'stop', 'damage', 'flashlight', 'door'.
    - argument params: set path arguments. First parameter is time to wait
      before action, and second is action duration.
    - create: create current path.
    - drawtype param: defines how to draw path. Param can be 'none' / 'all' /
      'next' or mix of: 'line', 'beam'.
    - info: display current path info on the console.
    - remove: remove current path.
    - removetype: remove current path type.

Item commands
-------------
Items are objects on the map, such as weapons, bullets, health-kits, armor,
buttons, doors, boxes, etc. They are defined in the MOD's configuration file.
Item commands start with a word "item".

    - draw param: defines WHICH items to draw. Param can be 'none' / 'all' /
      'next' or mix of: 'health', 'armor', 'weapon', 'ammo', 'button', 'door',
      'object', 'other'.
    - drawtype param: defines HOW to draw items. Param can be 'none' / 'all' /
      'next' or mix of: 'name', 'box', 'waypoint'.
    - reload: reload all items and recalculate nearest waypoints.

Bot commands
------------
Bot commands start with a word "bot".

    - add: add random bot.
    - debug param1 param2: show bot debug messages on the console. Param1 is
      bot's name and param2 can be 'on' or 'off'. When bot is in debug mode, the
      near items are drawn with rounding white box and nearest ones are drawn
      with red box.
    - drawpath param: defines how to draw bot's path. Param can be 'none' /
      'all' / 'next' or mix of: 'line', 'beam'.
    - kick: kick random bot or bot on team (given argument)
    - test: create bot to test path from given (current) to given (destination)
      waypoints
    - weapon: set of command to allow or forbid weapon use for bots. Weapons are
      defined in the MOD's configuration file.
        - allow params: allow bots to use given weapons.
        - forbid params: forbid bots to use given weapons.

Config commands
---------------
Config commands start with a word "config".

    - admins: set of commands to define admin's rights.
        - access param1 param2: set access flags for given admin. Param1 is
          Steam ID of the user, and param2 are access flags, and can be ' none'
          / 'all' or mix of: 'waypoint', 'bot', 'config'.
        - show: show admins currently on the server.
    - event param: display game events on the console. Param can be 'off' -
      disable, 'on' - enable.

