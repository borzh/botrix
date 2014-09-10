Source code of Botrix, bots plugin for Half-Life 2: Deathmatch.
===============================================================

Home page: http://www.famaf.unc.edu.ar/~godin/

Steps to compile
----------------

- Windows compilation:

        Microsoft Visual Studio 2010 with Service Pack 1 (at least)
        Download Git
        git clone https://github.com/ValveSoftware/source-sdk-2013.git source-sdk-2013
        git clone https://github.com/borzh/botrix botrix

- Linux compilation:

        sudo apt-get install git gcc-multilib ia32-libs cmake
        git clone https://github.com/ValveSoftware/source-sdk-2013.git source-sdk-2013
        git clone https://github.com/borzh/botrix botrix
        mkdir botrix/build
        cd botrix/build
        cmake ..
        make

