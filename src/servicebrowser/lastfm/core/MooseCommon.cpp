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

#include "MooseCommon.h"

#include "logger.h"
#include "UnicornCommon.h"
#include "LastFmSettings.h"

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QMap>
#include <QMessageBox>
#include <QPluginLoader>
#include <QProcess>

#ifdef WIN32
    #include "windows.h"
    #include "shlobj.h"
    #include <shellapi.h>
#endif

namespace MooseUtils
{


QString
dataPath( QString file )
{
    return QApplication::applicationDirPath() + "/data/" + file;
}


QString
savePath( QString file )
{
    QString path;

    #ifdef WIN32
        path = UnicornUtils::appDataPath();
        if ( path.isEmpty() )
            path = QApplication::applicationDirPath();
        else
            path += "/Last.fm/Client";
    #else
        path = UnicornUtils::appDataPath() + "/Last.fm";
    #endif

    QDir d( path );
    d.mkpath( path );

    return d.filePath( file );
}


QString
logPath( QString file )
{
    #ifndef Q_WS_MAC
        return savePath( file );
    #else
        return QDir( QDir::homePath() + "/Library/Logs" ).filePath( file );
    #endif

}


QString
cachePath()
{
    #ifdef Q_WS_MAC
        QString appSupportFolder = UnicornUtils::applicationSupportFolderPath();
        QDir cacheDir( appSupportFolder + "/Last.fm/Cache" );

        if (!cacheDir.exists())
        {
            cacheDir.mkpath( appSupportFolder + "/Last.fm/Cache" );
        }

        return cacheDir.path() + "/";
    #else
        return savePath( "cache/" );
    #endif
}


QString
servicePath( QString name )
{
    QString dirPath;
    #ifdef WIN32
        // Hack to get it working with VS2005
        dirPath = qApp->applicationDirPath();
    #else
        dirPath = qApp->applicationDirPath() + "/services";
    #endif

    #ifndef QT_NO_DEBUG
        dirPath += "/debug";
    #endif

    if ( name.isEmpty() )
    {
        return dirPath;
    }

    #ifndef QT_NO_DEBUG
        name += DEBUG_SUFFIX;
    #endif

    QDir servicesDir( dirPath );
    QString fileName = SERVICE_PREFIX + name + LIB_EXTENSION;

    return servicesDir.absoluteFilePath( fileName );
}


// This is the kind of beastly function that Max will throw a hissy fit if he can't have.
static void
loadServiceError( QString name )
{
    QMessageBox::critical( 0,
        QCoreApplication::translate( "Container", "Error" ),
        QCoreApplication::translate( "Container", "Couldn't load service: %1. The application won't be able to start." ).arg( name ) );

    QCoreApplication::exit( 1 );
}


QObject*
loadService( QString name )
{
    QString path = servicePath( name );

    qDebug() << "Loading service: " << name << "at" << path;

    QObject* plugin = QPluginLoader( path ).instance();

    if ( plugin == NULL )
    {
        loadServiceError( name );
        return NULL;
    }

    return plugin;
}


void
installHelperApp()
{
    if ( !The::settings().currentUser().isiPodScrobblingEnabled() )
        return;

    #ifdef Q_WS_MAC
        // Don't want to do this on Windows as it shuts down the helper and
        // then tries to launch it really quickly. Most of the time, the helper
        // has still not shut down when the call to relaunch it comes in with the
        // result that it doesn't launch.
        disableHelperApp();
    #endif
    enableHelperApp();
}


#ifndef Q_WS_X11
static QString
helperPath()
{
    QString path = qApp->applicationDirPath() + '/';

#ifdef WIN32
    #ifndef QT_NO_DEBUG
        path += "LastFmHelperd.exe";
    #else
        path += "LastFmHelper.exe";
    #endif
#endif

#ifdef Q_WS_MAC
    #ifndef QT_NO_DEBUG
        path += "LastFmHelper_debug";
    #else
        path += "LastFmHelper.app";
    #endif
#endif

    return path;
}
#endif


#ifdef WIN32
static QString
helperStartupShortcutPath()
{
#ifndef WIN32
    Q_ASSERT( !"Only implemented for Win" );
#else

    wchar_t acPath[MAX_PATH];
    HRESULT h = SHGetFolderPathW( NULL, CSIDL_STARTUP | CSIDL_FLAG_CREATE,
                                  NULL, 0, acPath );

    if ( h == S_OK )
    {
        QString shortcutPath;
        shortcutPath = QString::fromUtf16( reinterpret_cast<const ushort*>( acPath ) );
        if ( !shortcutPath.endsWith( "\\" ) && !shortcutPath.endsWith( "/" ) )
            shortcutPath += "\\";
        shortcutPath += "Last.fm Helper.lnk";
        return shortcutPath;
    }
    else
    {
        LOGL( 1, "Uh oh, Windows returned no STARTUP path, not going to be able to autolaunch helper" );
        return "";
    }
#endif
}
#endif


void
enableHelperApp()
{
    #ifdef Q_WS_MAC
        qDebug() << "enableHelperApp";

        CFArrayRef prefCFArrayRef = (CFArrayRef)CFPreferencesCopyValue( CFSTR( "AutoLaunchedApplicationDictionary" ), CFSTR( "loginwindow" ), kCFPreferencesCurrentUser, kCFPreferencesAnyHost );
        if ( prefCFArrayRef == NULL )
        {
            qDebug() << "enableHelperApp failed while getting prefCFArrayRef";
            prefCFArrayRef = CFArrayCreate( NULL, NULL, 0, NULL );
            if ( prefCFArrayRef == NULL )
            {
                qDebug() << "Can't create prefCFArrayRef";
            }
        }
        CFMutableArrayRef tCFMutableArrayRef = CFArrayCreateMutableCopy( NULL, 0, prefCFArrayRef );
        if ( tCFMutableArrayRef == NULL )
        {
            CFRelease( prefCFArrayRef );
            qDebug() << "enableHelperApp failed while getting tCFMutableArrayRef";
            return;
        }

        CFDictionaryRef dictRead = CFDictionaryCreate( NULL, NULL, NULL, 0, NULL, NULL );
        CFMutableDictionaryRef dict = CFDictionaryCreateMutableCopy( NULL, 0, dictRead );

        CFStringRef HideKey = CFStringCreateWithCString( NULL, "Hide", kCFStringEncodingASCII );
        CFBooleanRef HideValue = kCFBooleanFalse;
        CFStringRef PathKey = CFStringCreateWithCString( NULL, "Path", kCFStringEncodingASCII );
        CFStringRef path = CFStringCreateWithCharacters( 0, reinterpret_cast<const UniChar *>(helperPath().unicode()), helperPath().length() );

        CFDictionaryAddValue( dict, HideKey, HideValue );
        CFDictionaryAddValue( dict, PathKey, path );
        
//         CFDictionarySetValue( dict, CFSTR( "Path" ), path );
//         CFDictionarySetValue( dict, CFSTR( "Hide" ), kCFBooleanFalse );

        CFArrayAppendValue( tCFMutableArrayRef, dict );
        CFPreferencesSetValue( CFSTR( "AutoLaunchedApplicationDictionary" ), tCFMutableArrayRef, CFSTR( "loginwindow" ), kCFPreferencesCurrentUser, kCFPreferencesAnyHost );
        CFPreferencesSynchronize( CFSTR( "loginwindow" ), kCFPreferencesCurrentUser, kCFPreferencesAnyHost );

        CFRelease( path );
        CFRelease( dict );
        CFRelease( tCFMutableArrayRef );
        CFRelease( prefCFArrayRef );

        QProcess::startDetached( "open", QStringList() << helperPath() );

    #elif defined WIN32

        // On Windows, we create a shortcut and copy it to the user's Startup folder.
        QString helperApp = helperPath();
        QString shortcutPath = helperStartupShortcutPath();
        if ( !shortcutPath.isEmpty() )
        {
            HRESULT h = UnicornUtils::createShortcut(
                reinterpret_cast<const wchar_t*>( helperApp.utf16() ),
                L"", 
                reinterpret_cast<const wchar_t*>( shortcutPath.utf16() ) );

            if ( h != S_OK )
            {
                LOGL( 1, "Uh oh, Windows couldn't create a shortcut, not going to be able to autolaunch helper" );
            }
        }

        // Reason we need to launch it from code is because there are problems
        // with launching it from the installer on Vista after integrating the
        // VistaLib32.dll. See
        // www.codeproject.com/vista-security/RunNonElevated.asp?forumid=422222&select=2183086&df=100&msg=2183086

        // QProcess doesn't work on Vista
        HINSTANCE i = ShellExecuteW(
            NULL,
            L"open",
            reinterpret_cast<LPCWSTR>( helperApp.utf16() ),
            L"",
            NULL,
            SW_HIDE);

        if (reinterpret_cast<int>(i) <= 32) // Error
        {
            // Invalid handle means it didn't launch
            LOGL( 1, "Helper didn't launch: " << helperApp );
        }

    #endif
}


void
disableHelperApp()
{
    #ifdef Q_WS_MAC
        qDebug() << "disableHelperApp";

        CFArrayRef prefCFArrayRef = (CFArrayRef)CFPreferencesCopyValue( CFSTR( "AutoLaunchedApplicationDictionary" ), CFSTR( "loginwindow" ), kCFPreferencesCurrentUser, kCFPreferencesAnyHost );
        if (prefCFArrayRef == NULL) return;
        CFMutableArrayRef tCFMutableArrayRef = CFArrayCreateMutableCopy( NULL, 0, prefCFArrayRef );
        if (tCFMutableArrayRef == NULL) return;

        for ( int i = CFArrayGetCount( prefCFArrayRef ) - 1; i >= 0 ; i-- )
        {
            CFDictionaryRef dict = (CFDictionaryRef)CFArrayGetValueAtIndex( prefCFArrayRef, i );
            QString path = UnicornUtils::CFStringToQString( (CFStringRef) CFDictionaryGetValue( dict, CFSTR( "Path" ) ) );

            if ( path.toLower().contains( "lastfmhelper" ) )
            {
                qDebug() << "Removing helper from LoginItems at position" << i;
                CFArrayRemoveValueAtIndex( tCFMutableArrayRef, (CFIndex)i );
            }
        }

        CFPreferencesSetValue( CFSTR( "AutoLaunchedApplicationDictionary" ), tCFMutableArrayRef, CFSTR( "loginwindow" ), kCFPreferencesCurrentUser, kCFPreferencesAnyHost );
        CFPreferencesSynchronize( CFSTR( "loginwindow" ), kCFPreferencesCurrentUser, kCFPreferencesAnyHost );

        CFRelease( prefCFArrayRef );
        CFRelease( tCFMutableArrayRef );

        // LEGACY: disable old helper auto-launch!
        QString oldplist = QDir::homePath() + "/Library/LaunchAgents/fm.last.lastfmhelper.plist";
        if (QFile::exists( oldplist ))
        {
            QProcess::execute( "/bin/launchctl", QStringList() << "unload" << oldplist );
            QFile::remove( oldplist );
        }

        return;

    #elif defined WIN32

        // Delete the shortcut from startup and kill process
        QString helperApp = helperPath();
        QString shortcutPath = helperStartupShortcutPath();
        if ( !shortcutPath.isEmpty() )
        {
            bool fine = QFile::remove( shortcutPath );
            if ( !fine )
            {
                LOGL( 1, "Deletion of shortcut failed, helper will still autolaunch" );
            }
        }

        HINSTANCE i = ShellExecuteW(
            NULL,
            L"open",
            reinterpret_cast<LPCWSTR>( helperApp.utf16() ),
            L"--quit",
            NULL,
            SW_HIDE);

        if (reinterpret_cast<int>(i) <= 32) // Error
        {
            // Invalid handle means it didn't launch
            LOGL( 1, "Helper didn't shut down." );
        }

    #endif
}


QIcon
icon( const char *name )
{
    return QIcon( MooseUtils::dataPath( QString("icons/") + name + ".png" ) );
}


MooseEnums::ScrobblableStatus
scrobblableStatus( TrackInfo& track )
{
    using namespace MooseEnums;

    // Check duration
    if ( track.duration() < MooseConstants::kScrobbleMinLength )
    {
        LOG( 3, "Track length is " << track.duration() << " s which is too short, will not submit.\n" );
        return TooShort;
    }

    // Radio tracks above preview length always scrobblable
    if ( track.source() == TrackInfo::Radio )
    {
        return OkToScrobble;
    }

    // Check timestamp
    if ( track.timeStamp() == 0 )
    {
        LOG( 3, "Track has no timestamp, will not submit.\n" );
        return NoTimeStamp;
    }

    // Check if any required fields are empty
    if ( track.artist().isEmpty() )
    {
        LOG( 3, "Artist was missing, will not submit.\n" );
        return ArtistNameMissing;
    }
    if ( track.track().isEmpty() )
    {
        LOG( 3, "Artist, track or duration was missing, will not submit.\n" );
        return TrackNameMissing;
    }

    // Check if dir excluded
    if ( isDirExcluded( track.path() ) )
    {
        LOG( 3, "Track is in excluded directory '" << track.path() << "', " << "will not submit.\n" );
        return ExcludedDir;
    }

    QStringList invalidList;
    invalidList << "unknown artist"
                << "unknown"
                << "[unknown]"
                << "[unknown artist]";

    // Check if artist name is an invalid one like "unknown"
    foreach( QString invalid, invalidList )
    {
        if ( track.artist().toLower() == invalid )
        {
            LOG( 3, "Artist '" << track.artist() << "' is an invalid artist name, will not submit.\n" );
            return ArtistInvalid;
        }
    }

    // All tests passed!
    return OkToScrobble;
}


bool
isDirExcluded( const QString& path )
{
    QString pathToTest = QDir( path ).absolutePath();
    #ifdef WIN32
        pathToTest = pathToTest.toLower();
    #endif

    if (pathToTest.isEmpty())
        return false;

    foreach ( QString bannedPath, The::currentUser().excludedDirs() )
    {
        bannedPath = QDir( bannedPath ).absolutePath();
        #ifdef WIN32
            bannedPath = bannedPath.toLower();
        #endif

        // Try and match start of given path with banned dir
        if ( pathToTest.startsWith( bannedPath ) )
        {
            // Found, this path is from a banned dir
            return true;
        }
    }

    // The path wasn't found in exclusions list
    return false;
}


int
scrobbleTime( TrackInfo& track )
{
    // If we don't have a length or it's less than the minimum, return the
    // threshold
    if ( track.duration() <= 0 || track.duration() < MooseConstants::kScrobbleMinLength )
        return MooseConstants::kScrobbleTimeMax;

    float scrobPoint = qBound( MooseConstants::kScrobblePointMin,
                               The::currentUser().scrobblePoint(),
                               MooseConstants::kScrobblePointMax );
    scrobPoint /= 100.0f;

    return qMin( MooseConstants::kScrobbleTimeMax, int( track.duration() * scrobPoint ) );
}


} // namespace MooseUtils
