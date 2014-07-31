#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

#include "server_plugin.h"

int main(int, char **)
{
#ifdef _WIN32
    HMODULE handle = LoadLibrary("..\\..\\Release\\botrix.dll");
    if ( handle == NULL )
    {
        fprintf( stderr, "%Error reading dll.\n" );
        exit(1);
    }
    CreateInterfaceFn pFn = (CreateInterfaceFn)GetProcAddress( handle, "CreateInterface" );
#else
    void *handle = dlopen("/home/pccorar09/svn/source-sdk-2013/mp/game/mod_hl2mp/addons/botrix.so", RTLD_NOW);
    if ( !handle )
    {
        fprintf( stderr, "%s.\n", dlerror() );
        exit(1);
    }
    CreateInterfaceFn pFn = (CreateInterfaceFn)dlsym( handle, "CreateInterface" );
#endif

    if ( !pFn )
    {
        fprintf(stderr, "Can't load function from library.\n");
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        exit(1);
    }

    int iReturn;
    CBotrixPlugin* pPlugin = (CBotrixPlugin*)pFn(INTERFACEVERSION_ISERVERPLUGINCALLBACKS, &iReturn);
    if ( pPlugin && pPlugin->Load(pFn, pFn) )
    {
        pPlugin->LevelInit("dm_underpass");
    }

#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
}
