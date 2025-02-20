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

- Windows compilation:

        Microsoft Visual Studio 2019 (at least).
        Download Git.
        git clone git@github.com:ValveSoftware/source-sdk-2013.git
        git clone git@github.com:borzh/botrix.git

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

- Linux compilation:

        sudo apt-get install git build-essential gcc-multilib g++-multilib cmake
        # If you're building a legacy 32-bit version, you will also need ia32-libs or lib32z1
        git clone git@github.com:borzh/botrix.git
        mkdir botrix/build
        cd botrix/build
        cmake ..
        make
        
- After compile:

        Download botrix.zip from home page, unzip it to game directory (hl2mp/tf).
        Enter to build directory. In linux rename libbotrix.so to botrix.so.
        Move botrix.so (botrix.dll) to hl2mp/addons, replacing old files.

