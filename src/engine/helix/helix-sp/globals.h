
/*
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 */

/****************************************************************************
 * 
 *  Test Client:
 *  
 *
 *  This is an test client running on Windows, Mac, and Unix without a GUI.
 *
 */ 

#ifndef _SPLAY_GLOBALS_H
#define _SPLAY_GLOBALS_H

#include "dllpath.h"
#include "hxengin.h"
#include "hxcore.h"

typedef HX_RESULT (HXEXPORT_PTR FPRMSETDLLACCESSPATH) (const char*);

struct _stGlobals
{
    _stGlobals()
        : g_Players(NULL),
	  g_nPlayers(0),
	  bEnableAdviceSink(FALSE),
          bEnableVerboseMode(FALSE),
          g_pszUsername( NULL),
          g_pszPassword(NULL),
          g_pszGUIDFile(NULL),
          g_pszGUIDList(NULL),
          g_Error(HXR_OK),
	  g_ulNumSecondsPlayed(0),
	  pEngine(NULL),
          g_bNullRender(FALSE)
        {}

    IHXPlayer**          g_Players;
    int                  g_nPlayers;
    BOOL                 bEnableAdviceSink;
    BOOL                 bEnableVerboseMode;
    char*                g_pszUsername;
    char*                g_pszPassword;
    char*                g_pszGUIDFile;
    char*                g_pszGUIDList;
    HX_RESULT            g_Error;
    UINT32               g_ulNumSecondsPlayed;
    IHXClientEngine*     pEngine;
    BOOL                 g_bNullRender;
};


#endif // _SPLAY_GLOBALS_H
