/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef MOOSECOMMON_H
#define MOOSECOMMON_H

/** @author <erik@last.fm> */

#include "MooseDllExportMacro.h"

#include "TrackInfo.h"

#include <QString>
#include <QObject>

#ifdef QT_GUI_LIB
    #include <QIcon>
#endif

#ifdef WIN32
    #define EXTENSION_PREFIX "ext_"
    #define SERVICE_PREFIX "srv_"
    #define DEBUG_SUFFIX "d"
    #define LIB_EXTENSION ".dll"

#elif defined Q_WS_X11
    #define EXTENSION_PREFIX "libext_"
    #define SERVICE_PREFIX "libsrv_"
    #define DEBUG_SUFFIX "_debug"
    #define LIB_EXTENSION ".so"

#elif defined Q_WS_MAC
    #define EXTENSION_PREFIX "libext_"
    #define SERVICE_PREFIX "libsrv_"
    #define DEBUG_SUFFIX "_debug"
    #define LIB_EXTENSION ".dylib"
#endif

class LastFmSettings;
class WebService;

namespace The
{
    LastFmSettings& settings();
    WebService* webService();
}


namespace MooseEnums
{
    enum ScrobblableStatus
    {
        OkToScrobble,
        NoTimeStamp,
        TooShort,
        ArtistNameMissing,
        TrackNameMissing,
        ExcludedDir,
        ArtistInvalid
    };

    enum UserIconColour
    {
        eNone = -1,
        eRed = 0,
        eBlue,
        eGreen,
        eOrange,
        eBlack
    };
}


namespace MooseUtils
{
    /**
     * Returns path to named file in the app's bin/data directory.
     */
    MOOSE_DLLEXPORT QString
    dataPath( QString file = "" );

    /**
     * Returns path to named file in the app's user-writable save directory.
     * E.g. C:\Documents and Settings\user\Local Settings\Application Data\Last.fm\Client on Windows.
     *      ~/Library/Application Support/Last.fm/ on OS X.
     *      ~/.local/share/Last.fm/ on Unix.
     */
    MOOSE_DLLEXPORT QString
    savePath( QString file = "" );

    /**
     * Returns path to named file in the prefered (os depandant) logging directory.
     * E.g. C:\Documents and Settings\user\Local Settings\Application Data\Last.fm\Client on Windows.
     *      ~/Library/Logs/ on OS X.
     *      ~/.local/share/Last.fm/ on Unix.
     */
    MOOSE_DLLEXPORT QString
    logPath( QString file );

    /**
     * Returns path to directory for storing cached images etc.
     */
    MOOSE_DLLEXPORT QString
    cachePath();

    /**
     * Returns path to named service plugin.
     */
    MOOSE_DLLEXPORT QString
    servicePath( QString name );

    /**
     * Helper function to load up a named service plugin.
     */
    MOOSE_DLLEXPORT QObject*
    loadService( QString name );

    /**
     * On Mac, adds the helper to the autolaunched items,
     * on Windows, it just launches it.
     */
    MOOSE_DLLEXPORT void
    installHelperApp();

    /**
     * Enable helper autolaunching.
     */
    MOOSE_DLLEXPORT void
    enableHelperApp();

    /**
     * Stop the helper from autolaunching.
     */
    MOOSE_DLLEXPORT void
    disableHelperApp();

    /**
     * Helper function to load a named icon from disk.
     */
  #ifdef QT_GUI_LIB
    MOOSE_DLLEXPORT QIcon
    icon( const char *name );
  #endif

    /**
     * Checks whether the passed-in path is in a directory that the user has
     * excluded from scrobbling.
     */
    MOOSE_DLLEXPORT bool
    isDirExcluded( const QString& path );

    /**
     * Works out if passed-in track can be scrobbled and returns the status.
     */
    MOOSE_DLLEXPORT MooseEnums::ScrobblableStatus
    scrobblableStatus( TrackInfo& track );

    /**
     * Returns the second at which passed-in track reached the scrobble point.
     */
    MOOSE_DLLEXPORT int
    scrobbleTime( TrackInfo& track );

}


namespace MooseConstants
{
    // SCROBBLING CONSTANTS

    // The plugin ID used by HttpInput when submitting radio tracks to the player listener
    const QString kRadioPluginId = "radio";

    // Limits for user-configurable scrobble point (%)
    const int kScrobblePointMin = 50;
    const int kScrobblePointMax = 100;

    // Shortest track length allowed to scrobble (s)
    const int kScrobbleMinLength = 31;

    // Upper limit for scrobble time (s)
    const int kScrobbleTimeMax = 240;

    // Min size of buffer holding streamed http data, i.e the size the http
    // buffer needs to get to before we start streaming.
    const int kHttpBufferMinSize = 16 * 1024;

    // Max
    const int kHttpBufferMaxSize = 256 * 1024;

};


namespace MooseDefaults
{
    // Percentage of track length at which to scrobble
    const int kScrobblePoint = 50;

};


#endif // MOOSECOMMON_H
