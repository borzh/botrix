#!/bin/bash

ROOT_DIR="$(cd `dirname $0`/.. && pwd)"

STEAM_DIR=$(echo `cat /usr/bin/steam | grep "STEAMCONFIG=" | sed -e "s/STEAMCONFIG=//"`)
if [ "$STEAM_DIR" == "" ]; then
    STEAM_DIR=$HOME/.steam
fi

set -e -x

# Steam is i386 (32bit).
STEAM_RUNTIME="$HOME/.steam/ubuntu12_32/steam-runtime"
export LD_LIBRARY_PATH="$GAME_DIR/bin:$STEAM_RUNTIME/i386/lib/i386-linux-gnu:$STEAM_RUNTIME/i386/lib:$STEAM_RUNTIME/i386/usr/lib/i386-linux-gnu:$STEAM_RUNTIME/i386/usr/lib:$STEAM_RUNTIME/amd64/lib/x86_64-linux-gnu:$STEAM_RUNTIME/amd64/lib:$STEAM_RUNTIME/amd64/usr/lib/x86_64-linux-gnu:$STEAM_RUNTIME/amd64/usr/lib:$LD_LIBRARY_PATH"

# Check out command line parameters.

if [ "$1" == "hl2dm" ]; then
    GAME_DIR="$STEAM_DIR/steam/SteamApps/common/Half-Life 2 Deathmatch"
    MOD_DIR="$STEAM_DIR/steam/SteamApps/common/Half-Life 2 Deathmatch/hl2mp"

    mkdir -p "$MOD_DIR/addons" || true
    cp "$ROOT_DIR/source-sdk-2013/mp/game/mod_hl2mp/addons"/* "$MOD_DIR/addons/"

    cd "$GAME_DIR"
    ./hl2_linux -insecure -allowdebug -novid -console -game "$MOD_DIR"
elif [ "$1" == "srcds" ]; then
    GAME_DIR="$STEAM_DIR/steam/SteamApps/common/Source SDK Base 2013 Dedicated Server"
    MOD_DIR="$GAME_DIR/hl2mp"

    mkdir -p "$MOD_DIR/addons" || true
    cp "$ROOT_DIR/source-sdk-2013/mp/game/mod_hl2mp/addons"/* "$MOD_DIR/addons/"

    cd "$GAME_DIR"
    ./srcds_linux -insecure -allowdebug -novid -console -game "$MOD_DIR"
else
    GAME_DIR="$STEAM_DIR/steam/SteamApps/common/Source SDK Base 2013 Multiplayer"
    MOD_DIR="$ROOT_DIR/source-sdk-2013/mp/game/mod_hl2mp"

    cd "$GAME_DIR"
    ./hl2_linux -game "$MOD_DIR" -debug 
fi

