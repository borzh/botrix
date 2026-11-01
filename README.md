Source code of Botrix, bots plugin for Half-Life 2: Deathmatch.
===============================================================

Home page: http://www.famaf.unc.edu.ar/~godin/botrix


Plugin's demo videos on YouTube:
----------------
- [General gameplay](http://www.youtube.com/watch?v=6MCQTqh8Z9c).
- [Waypoints](http://www.youtube.com/watch?v=rDhOGZde0s4).
- [Bot's executing a plan](http://www.youtube.com/watch?v=ciSjeTX-0gI).


Steps to compile
----------------

- Windows compilation (New SDK 2013 version / x64):

        I updated to Microsoft Visual Studio 2019.
        Download Git.
        git clone git@github.com:ValveSoftware/source-sdk-2013.git
        git clone git@github.com:borzh/botrix.git
        cd botrix
        Launch "Botrix.sln"

- Windows compilation (Old SDK 2013 version, 32-bit / x86):

        Microsoft Visual Studio 2013.
        Download Git.
        git clone git@github.com:ValveSoftware/source-sdk-2013.git
        git clone git@github.com:borzh/botrix.git

        IMPORTANT: the 32-bit (x86) build needs an OLD revision of the SDK,
        as they dropped all 32-bit Windows libraries, so a fresh clone can no
        longer link an x86 build.
        Check out the last revision that still ships the 32-bit libs:

        cd source-sdk-2013
        git checkout 0d8dceea4310fde5706b3ce1c70609d72a38efdf

        cd ../botrix
        Launch "Botrix.sln"
  
- OSX compilation:

        macOS 10.15 Catalina and later (no need for macOS 10.14 Mojave and earlier):
            Download MacOSX10.13.sdk.tar.xz from https://github.com/phracker/MacOSX-SDKs/releases
            (or svn checkout https://github.com/phracker/MacOSX-SDKs/trunk/MacOSX10.13.sdk)
            mv MacOSX10.13.sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/
        
        Install CMake, like: brew install cmake
        
        git clone git@github.com:ValveSoftware/source-sdk-2013.git
        git clone git@github.com:borzh/botrix.git
        mkdir botrix/build
        cd botrix/build
        cmake ..
        make

- Linux compilation (New Source 2013 update):

        sudo apt-get install git build-essential cmake
        git clone git@github.com:borzh/botrix.git
        mkdir botrix/build
        cd botrix/build
        cmake ..
        make

- Linux compilation (Legacy Source 2013):
        
        sudo apt-get install git build-essential cmake gcc-multilib g++-multilib
        git clone git@github.com:borzh/botrix.git
        mkdir botrix/build
        cd botrix/build
        cmake -DLEGACY_32BIT=1 ..
        make
          
        
- After compile:

        Download botrix.zip from home page, unzip it to game directory (hl2mp/tf).
        Move botrix.so (botrix.dll) to hl2mp/addons, replacing old files.

