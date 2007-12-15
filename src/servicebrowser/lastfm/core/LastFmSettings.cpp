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

#include "LastFmSettings.h"

#include "UnicornCommon.h" // md5Digest
#include "MooseCommon.h"
#include "Settings.h"
#include "Station.h"
#include "logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#ifdef Q_WS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif



/******************************************************************************
 * LastFmUserSettings
 ******************************************************************************/
MooseEnums::UserIconColour
LastFmUserSettings::icon() const
{
    MyQSettings s( this );

    // This will return eRed if there is no entry. Don't want that.
    if (s.contains( "Icon" ))
        return (MooseEnums::UserIconColour) s.value( "Icon" ).toInt();
    else
        return MooseEnums::eNone; 
}


void
LastFmUserSettings::setIcon( MooseEnums::UserIconColour colour )
{
    MyQSettings( this ).setValue( "Icon", static_cast<int>(colour) );
    emit userChanged( username() );
}


/// Written as int for backwards compatibility with the MFC Audioscrobbler
bool
LastFmUserSettings::isLogToProfile() const
{
    return static_cast<bool>(MyQSettings( this ).value( "LogToProfile", 1 ).toInt());
}


/// Written as int for backwards compatibility with the MFC Audioscrobbler
void
LastFmUserSettings::setLogToProfile( bool state )
{
    MyQSettings( this ).setValue( "LogToProfile", static_cast<int>(state) );
    emit userChanged( username() );
}


bool
LastFmUserSettings::isDiscovery() const
{
    return MyQSettings( this ).value( "DiscoveryEnabled", false ).toBool();
}


void
LastFmUserSettings::setDiscovery( bool state )
{
    MyQSettings( this ).setValue( "DiscoveryEnabled", state );
    emit userChanged( username() );
}


bool
LastFmUserSettings::sidebarEnabled() const
{
    return MyQSettings( this ).value( "SidebarEnabled", false ).toBool();
}


void
LastFmUserSettings::setSidebarEnabled( bool state )
{
    MyQSettings( this ).setValue( "SidebarEnabled", state );
    emit userChanged( username() );
}


void
LastFmUserSettings::setResumePlayback( bool enabled )
{
    MyQSettings( this ).setValue( "resumeplayback", enabled ? "1" : "0" );
    emit userChanged( username() ); 
}


void
LastFmUserSettings::setResumeStation( StationUrl station )
{
    MyQSettings( this ).setValue( "resumestation", station );
    emit userChanged( username() );
}


void
LastFmUserSettings::addRecentStation( const Station& station )
{
    MyQSettings s( this );

    QList<Station> stations = recentStations();

    // remove duplicates
    for ( int i = 0; i < stations.count(); ++i )
        if ( stations[i].url() == station.url() )
            stations.removeAt( i-- );

    stations.prepend( station );

    s.remove( "RecentStations" );

    s.beginGroup( "RecentStations" );
    int j = stations.count();
    while (j--)
        s.setValue( QString::number( j ), stations[j].url() );
    s.endGroup();

    s.setValue( "StationNames/" + station.url(), station.name() );
    s.sync();

    emit userChanged( username() );
    emit historyChanged();
}


void
LastFmUserSettings::removeRecentStation( int n )
{
    MyQSettings s( this );

    QString const N = QString::number( n );

    s.beginGroup( "RecentStations" );
    QString const url = s.value( N ).toString();
    s.remove( N );

    // now renumber in correct order (maps are auto-sorted by key)
    QMap<int, QString> urls;
    foreach (QString key, s.childKeys())
        urls[key.toInt()] = s.value( key ).toString();

    s.remove( "" ); //current group

    int i = 0;
    foreach (QString url, urls)
        s.setValue( QString::number( i++ ), url );
    s.endGroup();

    s.remove( "StationNames/" + url );
    s.sync();

    emit userChanged( username() );
    emit historyChanged();
}


void
LastFmUserSettings::clearRecentStations( bool emitting )
{
    MyQSettings( this ).remove( "RecentStations" );

    //TODO needed still?
    if ( emitting )
        emit historyChanged();
}


QList<Station>
LastFmUserSettings::recentStations()
{
    MyQSettings s( this );

    s.beginGroup( "RecentStations" );
    QStringList const keys = s.childKeys();
    s.endGroup();

    QMap<int, Station> stations;
    foreach (QString key, keys) {
        Station station;
        station.setUrl( s.value( "RecentStations/" + key ).toString() );
        station.setName( s.value( "StationNames/" + station.url() ).toString() );
        stations[key.toInt()] = station;
    }

    return stations.values();
}


void
LastFmUserSettings::setExcludedDirs( QStringList dirs )
{
    #ifdef WIN32
        QMutableStringListIterator i( dirs );
        while (i.hasNext()) {
            QString& s = i.next();
            s = s.toLower();
        }
    #endif

    MyQSettings( this ).setValue( "ExclusionDirs", dirs );
    emit userChanged( username() );
}


QStringList
LastFmUserSettings::excludedDirs() const
{
    QStringList paths = MyQSettings( this ).value( "ExclusionDirs" ).toStringList();
    paths.removeAll( "" );

    return paths;
}


void
LastFmUserSettings::setIncludedDirs( QStringList dirs )
{
    MyQSettings( this ).setValue( "InclusionDirs", dirs );
    emit userChanged( username() );
}


QString
LastFmUserSettings::bootStrapPluginId() const
{
    return MyQSettings( this ).value( "BootStrapPluginId" ).toString();
}


void
LastFmUserSettings::setBootStrapPluginId( QString id )
{
    MyQSettings( this ).setValue( "BootStrapPluginId", id );
    emit userChanged( username() );
}


QStringList
LastFmUserSettings::includedDirs() const
{
    return MyQSettings( this ).value( "InclusionDirs" ).toStringList();
}


void
LastFmUserSettings::setMetaDataEnabled( bool enabled )
{
    MyQSettings( this ).setValue( "DownloadMetadata", enabled );
    emit userChanged( username() );
}


bool
LastFmUserSettings::isMetaDataEnabled()
{
    return MyQSettings( this ).value( "DownloadMetadata", true ).toBool();
}


void
LastFmUserSettings::setCrashReportingEnabled( bool enabled )
{
    MyQSettings( this ).setValue( "ReportCrashes", enabled );
    emit userChanged( username() );
}


bool
LastFmUserSettings::crashReportingEnabled()
{
    return MyQSettings( this ).value( "ReportCrashes", true ).toBool();
}


void
LastFmUserSettings::setScrobblePoint( int scrobblePoint )
{
    MyQSettings( this ).setValue( "ScrobblePoint", scrobblePoint );
    emit userChanged( username() );
}


int
LastFmUserSettings::scrobblePoint()
{
    return MyQSettings( this ).value( "ScrobblePoint", MooseDefaults::kScrobblePoint ).toInt();
}


void
LastFmUserSettings::setFingerprintingEnabled( bool enabled )
{
    MyQSettings( this ).setValue( "Fingerprint", enabled );
    emit userChanged( username() );
}


bool
LastFmUserSettings::fingerprintingEnabled()
{
    return MyQSettings( this ).value( "Fingerprint", true ).toBool();
}


void
LastFmUserSettings::setTrackFrameClockMode( bool trackTimeEnabled )
{
    MyQSettings( this ).setValue( "TrackFrameShowsTrackTime", trackTimeEnabled );
    emit userChanged( username() );
}


bool
LastFmUserSettings::trackFrameClockMode()
{
    return MyQSettings( this ).value( "TrackFrameShowsTrackTime", true ).toBool();
}


// Don't rename registry key. Used by player plugins!
void
LastFmUserSettings::setLaunchWithMediaPlayer( bool en )
{
    MyQSettings( this ).setValue( "LaunchWithMediaPlayer", en );
    emit userChanged( username() );
}


bool
LastFmUserSettings::launchWithMediaPlayer()
{
    return MyQSettings( this ).value( "LaunchWithMediaPlayer", true ).toBool();
}


void
LastFmUserSettings::setiPodScrobblingEnabled( bool en )
{
    MyQSettings( this ).setValue( "iPodScrobblingEnabled", en );
    emit userChanged( username() );
}


bool
LastFmUserSettings::isiPodScrobblingEnabled()
{
    return MyQSettings( this ).value( "iPodScrobblingEnabled", true ).toBool();
}


/******************************************************************************
 *   Settings
 ******************************************************************************/
LastFmSettings::LastFmSettings( QObject* parent ) :
    AppSettings<QSettings>( parent ),
    m_nullUser( "" )
{
    #ifndef WIN32
        QSettings new_config;

        if (!QFile( new_config.fileName() ).exists())
        {
            //attempt to upgrade settings object from old and broken location
            foreach (QString const name, QStringList() << "Client" << "Users" << "Plugins" << "MediaDevices")
            {
                QSettings old_config( QSettings::IniFormat, QSettings::UserScope, "Last.fm", name );
                old_config.setFallbacksEnabled( false );

                if (!QFile::exists( old_config.fileName() ))
                    continue;

                foreach (QString const key, old_config.allKeys()) {
                    if (name != "Client")
                        //Client now becomes [General] group as this makes most sense
                        new_config.beginGroup( name );
                    new_config.setValue( key, old_config.value( key ) );
                    #ifndef QT_NO_DEBUG
                    if (name != "Client") // otherwise qWarning and aborts
                    #endif
                    new_config.endGroup();
                }

                new_config.sync();

                QFile f( old_config.fileName() );
                f.remove();
                QFileInfo( f ).dir().rmdir( "." ); //safe as won't remove a non empty dir
            }
        }
    #endif

    s_instance = this;
}


LastFmUserSettings&
LastFmSettings::user( QString username ) const
{
    Q_ASSERT( username != "" );

    LastFmUserSettings *user = findChild<LastFmUserSettings*>( username );

    if (!user) {
        user = new LastFmUserSettings( username );
        user->setParent( const_cast<LastFmSettings*>(this) );
        user->setObjectName( username );
        connect( user, SIGNAL(userChanged( QString )), SLOT(userChanged( QString )) );
    }

    return *user;
}


LastFmUserSettings&
LastFmSettings::currentUser()
{
    return currentUsername() == ""
            ? m_nullUser
            : user( currentUsername() );
}


void
LastFmSettings::setCurrentUsername( QString username ) 
{
    UsersSettings<QSettings>().setValue( CURRENT_USER_KEY, username );

    emit userSettingsChanged( currentUser() );
    emit userSwitched( currentUser() );
}


bool
LastFmSettings::deleteUser( QString username )
{
    if (isExistingUser( username ))
    {
        delete &user( username );
        UsersSettings<QSettings>().remove( username );

        return true;
    }
    else
        return false;
}


int
LastFmSettings::externalSoundSystem()
{
    int externalSystem = -1;
    #ifdef WIN32
    externalSystem = 1;
    #endif
    #ifdef Q_WS_X11
    externalSystem = 2;
    #endif
    #ifdef Q_WS_MAC
    externalSystem = 1;
    #endif

    return externalSystem;
}


void
LastFmSettings::setDontAsk( const QString op, bool value )
{
    QSettings().setValue( op + "DontAsk", value );
}


bool
LastFmSettings::isDontAsk( const QString op ) const
{
    return QSettings().value( op + "DontAsk" ).toBool();
}


void
LastFmSettings::setShowTrayIcon( bool en )
{
    QSettings().setValue( "ShowTrayIcon", en );
    emit appearanceSettingsChanged();
}


bool
LastFmSettings::isFirstRun() const
{
    // We fallback on HKLM here as versions of the client prior to 1.3.2
    // stored the value there
    QSettings s;
    if ( s.contains( "FirstRun" ) )
        return s.value( "FirstRun", "1" ).toBool();
    else
        return HklmSettings().value( "FirstRun", "1" ).toBool();
}


void
LastFmSettings::setFirstRunDone()
{
    QSettings().setValue( "FirstRun", "0" );
}


QStringList
LastFmSettings::allPlugins( bool withVersions )
{
    // These valued are written by the plugin installers and hence live in
    // PluginsSettings, i.e. HKLM.

    PluginsSettings s;
    QStringList plugins;

    foreach (QString group, s.childGroups()) {
        s.beginGroup( group );
        QString name = s.value( "Name" ).toString();


        //If the plugin has been added but not installed name.size() == 0
        if ( name.size() != 0 )
        {
            if ( withVersions )
            {
                QString version = s.value( "Version" ).toString();
                plugins += name + ' ' + tr("plugin, version") + ' ' + version;
            }
            else
            {
                plugins += name;
            }
        }
        s.endGroup();
    }

    return plugins;
}


QString
LastFmSettings::pluginVersion( QString id )
{
    Q_ASSERT( !id.isEmpty() );

    // These valued are written by the plugin installers and hence live in
    // PluginsSettings, i.e. HKLM.

    return PluginsSettings().value( id + "/Version" ).toString();
}


QString
LastFmSettings::pluginPlayerPath( QString id )
{
    Q_ASSERT( !id.isEmpty() );

    QString key = "Plugins/" + id + "/PlayerPath";

    // We fallback on HKLM here as versions of the client prior to 1.3.2
    // stored the value there
    QSettings s;
    if ( s.contains( key ) )
        return s.value( key, "" ).toString();
    else
        return HklmSettings().value( key, "" ).toString();
}


void
LastFmSettings::setPluginPlayerPath( QString id,
                               QString path )
{
    Q_ASSERT( !id.isEmpty() );

    // Does not go in PluginsSettings since that's in HKLM (written by the installer),
    // and a standard user does not have write access to that.
    QSettings().setValue( "Plugins/" + id + "/PlayerPath", path );
}


QStringList
LastFmSettings::allMediaDevices()
{
    MediaDeviceSettings s;
    return s.childGroups();
}


QString
LastFmSettings::mediaDeviceUser( QString uid ) const
{
    MediaDeviceSettings s;
    s.beginGroup( uid );
    return s.value( "user" ).toString();
}


void
LastFmSettings::addMediaDevice( QString uid, QString username )
{
    MediaDeviceSettings s;
    s.beginGroup( uid );
    s.setValue( "user", username );
    s.sync();
}


void
LastFmSettings::removeMediaDevice( QString uid )
{
    MediaDeviceSettings s;
    s.beginGroup( uid );
    s.remove( "user" );
    s.sync();
}


MooseEnums::UserIconColour
LastFmSettings::getFreeColour()
{
    UsersSettings<QSettings> s;
    QList<int> unused;

    // Fill it with all colours
    for (int i = 0; i < 5; ++i)
    {
        unused.push_back(i);
    }

    // Remove the ones in use
    foreach (QString username, s.childGroups())
    {
        MooseEnums::UserIconColour col = LastFmUserSettings( username ).icon();

        if (col != MooseEnums::eNone)
            unused.removeAll( int(col) );

        if (unused.isEmpty()) {
            LOG( 2, "We ran out of colours, returning random\n" );
            return static_cast<MooseEnums::UserIconColour>(rand() % 5);
        }
    }

    return static_cast<MooseEnums::UserIconColour>(unused.front());
}


void
LastFmSettings::userChanged( QString username )
{
    if ( username == currentUsername() )
        emit userSettingsChanged( currentUser() );
}
