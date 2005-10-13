#include <iostream>
#include <string>
#include <dlfcn.h>

#include "hxsplay.h"

using namespace std;

#ifdef TEST_APP
char* GetAppName(char* pszArgv0)
{
    char* pszAppName;

    pszAppName = strrchr(pszArgv0, '\\');

    if (NULL == pszAppName)
    {
        return pszArgv0;
    }
    else
    {
        return pszAppName + 1;
    }
}

void PrintUsage(const char* pszAppName)
{
    cout << "\n";

#if defined _DEBUG || defined DEBUG
    cout << "USAGE:\n" << pszAppName << "[-as0] [-d D] [-n N] [-t T] [-st ST] [-g file] [-u username] [-p password] <URL>\n";
#else
    cout << "USAGE:\n" << pszAppName << "[-as0] [-n N] [-t T] [-g file] [-u username] [-p password] <URL>\n";
#endif
    cout << "       -a : optional flag to show advise sink output\n";
    cout << "       -s : optional flag to output useful status messages\n";
    cout << "       -0 : optional flag to disable all output windows\n";
#ifndef _STATICALLY_LINKED
    cout << "       -l : optional flag to tell the player where to find its DLLs\n";
#endif

#if defined _DEBUG || defined DEBUG
    cout << "       -d : HEX flag to print out DEBUG info\n";
    cout << "            0x8000 -- for audio methods calling sequence\n"
            "0x0002 -- for variable values\n";
#endif
    cout << "       -n : optional flag to spawn N players\n";
    cout << "       -rn: optional flag to repeat playback N times\n";
    cout << "\n";
}

int main( int argc, char *argv[] )
{
   int   i;
   int   nNumPlayers = 1;
   int   nNumPlayRepeats = 1;
   int   nTimeDelta;
   int   nStopTime;
   bool  bURLFound = false;

    //See if the user has set their HELIX_LIBS env var. This is overridden by the
    //-l option.
    int volscale = 1;

    for (i = 1; i < argc; i++)
    {
        if (0 == strcmp(argv[i], "-a"))
        {
        }
        else if (0 == strcmp(argv[i], "-s"))
        {
        }
        else if (0 == strcmp(argv[i], "-0"))
        {
        }
        else if (0 == strcmp(argv[i], "-v"))
        {
           if (++i == argc)
           {
              cout << "\nError: Invalid value for -n option.\n\n";
              PrintUsage(GetAppName(argv[0]));
              return -1;
           }
           volscale = atoi(argv[i]);
        }
        else if (0 == strcmp(argv[i], "-n"))
        {
            if (++i == argc)
            {
                cout << "\nError: Invalid value for -n option.\n\n";
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
            nNumPlayers = atoi(argv[i]);
            if (nNumPlayers < 1)
            {
                cout << "\nError: Invalid value for -n option.\n\n";
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
        }
        else if (0 == strcmp(argv[i], "-rn"))
        {
            if (++i == argc)
            {
                cout << "\nError: Invalid value for -rn option.\n\n";
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
            nNumPlayRepeats = atoi(argv[i]);
            if (nNumPlayRepeats < 1)
            {
                cout << "\nError: Invalid value for -rn option.\n\n";
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
        }
        else if (0 == strcmp(argv[i], "-t"))
        {
            if (++i == argc)
            {
                cout << "\nError: Invalid value for -t option.\n\n";
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
            nTimeDelta = atoi(argv[i]);
            if (nTimeDelta < 0)
            {
                cout << "\nError: Invalid value for -t option.\n\n";
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
        }
        else if (0 == strcmp(argv[i], "-st"))
        {
            if (++i == argc)
            {
                cout << "\nError: Invalid value for -st option.\n\n";
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
            nStopTime = atoi(argv[i]);
            if (nStopTime < 0)
            {
                cout << "\nError: Invalid value for -st option.\n\n";
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
        }
#if defined _DEBUG || defined DEBUG
        else if (0 == strcmp(argv[i], "-d"))
        {
            if (++i == argc)
            {
		cout << "\nError: Invalid value for -d option.\n\n";
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
        }
#endif
        else if (0 == strcmp(argv[i], "-u"))
        {
            char *puser = new char[1024];
            strcpy(puser, ""); /* Flawfinder: ignore */
            if (++i == argc)
            {
                cout << "\nError: Invalid value for -u option.\n\n";
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
        }
        else if (0 == strcmp(argv[i], "-p"))
        {
           char *ppass = new char[1024];
            strcpy(ppass, ""); /* Flawfinder: ignore */
            if (++i == argc)
            {
                cout << "\nError: Invalid value for -p option.\n\n";
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
        }
        else if (0 == strcmp(argv[i], "-g"))
        {
           char *pfile = new char[1024];
            strcpy(pfile, ""); /* Flawfinder: ignore */
            if (++i == argc)
            {
                cout << "\nError: Invalid value for -g option.\n\n";
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
        }
        else if (0 == strcmp(argv[i], "-l"))
        {
            if (++i == argc)
            {
                cout << "\nError: Invalid value for -l option.\n\n";
                PrintUsage(GetAppName(argv[0]));
                return -1;
            }
        }
        else if (!bURLFound)
        {
            bURLFound  = true;
            //if no "://" was found lets add file:// by default so that you
            //can refer to local content as just ./splay ~/Content/startrek.rm,
            //for example, and not ./splay file:///home/gregory/Content/startrek.rm
        }
        else
        {
            PrintUsage(GetAppName(argv[0]));
            return -1;
        }
    }

    if (!bURLFound)
    {
        if (argc > 1)
        {
           cout << "\nError: No media file or URL was specified.\n\n";
        }
        PrintUsage(GetAppName(argv[0]));
        return -1;
    }

    HXSplay splay;
    splay.init(HELIX_DIR "/common",
               HELIX_DIR "/plugins",
               HELIX_DIR "/codecs");
    splay.setURL(argv[i-1]);

    sleep(5);
    cerr << "Playback time: " << splay.duration(0) << endl;
    splay.play();

    int vol, pos;
    while (!splay.done())
    {
       pos = splay.where(0);
       cerr << "Playback at " << pos << " of " << splay.duration(0) << endl;
       vol = splay.getVolume(0);
       cerr << "Vol is " << vol << endl;
       sleep(10);
    }
}
#endif // TEST_APP
