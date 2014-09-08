Botrix
======
Botrix (from BOT's matRIX) is a plugin for Valve Source Engine games that allows
to play with bots. Currently supported games:
* Half-Life 2: Deathmatch
* Team Fortress 2 (arena maps)


Changelog
=========
0.0.3
* Plugin is working with Team Fortress 2.
* Added new weapons handling in config.ini for TF2.
* Escaping TF2 steam ids in config.ini, for example \\[U:1:12345678].
* Bug fix: correct handling of plugin_pause/plugin_unpause commands.
* Bug fix: sometimes bot wasn't aware it picked up item.
* Bug fix: sometimes bot was shooting at spectator.
* Bug fix: when bot was trying to use weapon he actually was creating it.
* Bug fix: waypoints wasn't loading for maps with different case (linux).
* Now bots can use melee weapons.
* Now bots can use unknown weapons.
* Added logic for bot to pursue enemy.
* Bots will run randomly near engaged enemy, preferently using visible areas.
* Added new commands:
*     botrix bot config quota <#number/#player-#bot quota>
*     botrix bot config intelligence <bot-intelligence>
*     botrix bot config team <bot-team>
*     botrix bot config class <bot-class>
*     botrix bot config change-class <round-limit>
*     botrix bot config strategy flags
*     botrix bot config strategy set
*     botrix bot command <bot-name> <command>
*     botrix bot weapon add <bot-name> <weapon-name>
*     botrix bot weapon unknown <melee/ranged>.
*     botrix config log <log-level>
*     botrix enable/disable
* Fixed commands:
*     botrix bot kick <bot-name/all>
* Waypoint edition: aim at waypoint to select 'path destination'.

0.0.2
* Works in Linux.
* Default base folder location is <MOD DIRECTORY>/addons/botrix.
* Searching for base folder in 4 different locations.
* Log with levels (none, trace, debug, info, warning, error).
* Added command "version".
* Don't repeat bot names.
* Fixed several crashes.
 

Credits
=======
* Plugin's grandfather is Botman's bot template (aka HPB_bot). Thanks grandpa!
* Botrix was created as a thesis for an academic degree. Thanks a lot to my
  thesis director to give me a chance to make something I like.
* Plugin is working on Team Fortress 2 because of HappoKala345 (aka AcidFish)
  request. It also works with VSH plugin, for which manual weapon handling was
  added. Many of HappoKala's ideas were used, thanks a lot for waypoints 
  creation and testing!


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
- Bots can't chat.
- Handling of medic/ingineer 'weapons'.
- Support for other mods.

