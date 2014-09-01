Source code of Botrix, bots plugin for Half-Life 2: Deathmatch.
===============================================================

Home page: http://www.famaf.unc.edu.ar/~godin/

Steps to compile
----------------

- First clone Source SDK 2013:

        git clone https://github.com/ValveSoftware/source-sdk-2013.git source-sdk-2013

- Windows compilation (at least):

        Microsoft Visual Studio 2010 with Service Pack 1

- Linux compilation:

        sudo apt-get install gcc-multilib ia32-libs cmake
        mkdir build
        cd build
        cmake ..
        make

