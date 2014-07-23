#!/bin/bash

set -e

ROOT_DIR="$(cd `dirname $0`/.. && pwd)"

STEAM_DIR=$(echo `cat /usr/bin/steam | grep "STEAMCONFIG=" | sed -e "s/STEAMCONFIG=//"`)
GAME_DIR="$STEAM_DIR/steam/SteamApps/common/Source SDK Base 2013 Multiplayer"
MOD_DIR="$ROOT_DIR/source-sdk-2013/mp/game/mod_hl2mp"

# Steam is i386 (32bit).
STEAM_RUNTIME="$HOME/.steam/ubuntu12_32/steam-runtime"
export LD_LIBRARY_PATH="$GAME_DIR/bin:$STEAM_RUNTIME/i386/lib/i386-linux-gnu:$STEAM_RUNTIME/i386/lib:$STEAM_RUNTIME/i386/usr/lib/i386-linux-gnu:$STEAM_RUNTIME/i386/usr/lib:$STEAM_RUNTIME/amd64/lib/x86_64-linux-gnu:$STEAM_RUNTIME/amd64/lib:$STEAM_RUNTIME/amd64/usr/lib/x86_64-linux-gnu:$STEAM_RUNTIME/amd64/usr/lib:$LD_LIBRARY_PATH"

cd "$GAME_DIR"
./hl2_linux -insecure -allowdebug -novid -console -game $MOD_DIR

