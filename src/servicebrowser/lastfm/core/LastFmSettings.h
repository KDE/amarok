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

#ifndef LASTFMSETTINGS_H
#define LASTFMSETTINGS_H

#include "Settings.h"

#include "MooseDllExportMacro.h"

#include "StationUrl.h"
#include "MooseCommon.h"

#include <QMap>
#include <QPoint>
#include <QSettings>
#include <QSize>
#include <QStringList>
#include <QMutex>
#include <QMutexLocker>

static const int kDefaultControlPort = 32213;
static const int kDefaultHelperControlPort = 32223;


#if 1
#  define CURRENT_USER_KEY "CurrentUser"
#else
#  define CURRENT_USER_KEY "DebugBuildCurrentUser"
#endif

/**
 * Note we operate on separate QSettings objects always to be thread-safe.
 * Note the silly classes for each group are because you can't copy QSettings objects
 * so we couldn't just return one through a function, which would be neater.
 */

#ifdef WIN32
    class HklmSettings : public QSettings
    {
    public:
        HklmSettings( QObject* parent = 0 ) :
                QSettings( "HKEY_LOCAL_MACHINE\\Software\\Last.fm\\Client",
                           QSettings::NativeFormat,
                           parent )
        {}
    };
    
    /** Due to historical reasons, we store windows settings "wrongly", but
      * migrate settings on other platforms to the "correct" location. To make
      * the code all the same though we use this class below and macro it to
      * the QSettings token */
    class HkcuSettings : public QSettings
    {
    public:
        HkcuSettings( QObject* parent = 0 ) :
            QSettings( "HKEY_CURRENT_USER\\Software\\Last.fm\\Client",
                       QSettings::NativeFormat,
                       parent )
        {}
    };
    
    // set QSettings to HkcuSettings
    #define QSettings HkcuSettings
#else
    // set HklmSettings to QSettings
    typedef QSettings HklmSettings;
#endif // WIN32


class PluginsSettings : public HklmSettings
{
public:
    PluginsSettings()
    {
        beginGroup( "Plugins" );
    }
};


class MediaDeviceSettings : public QSettings
{
public:
    MediaDeviceSettings()
    {
        beginGroup( "MediaDevices" );
    }
};


class MOOSE_DLLEXPORT LastFmUserSettings : public UserSettings<QSettings>
{
    Q_OBJECT
    
public:
    LastFmUserSettings( const QString& username ) :
        UserSettings<QSettings>( username ) { }

    MooseEnums::UserIconColour icon() const;
    void setIcon( MooseEnums::UserIconColour colour );

    bool isLogToProfile() const;
    void setLogToProfile( bool state );
    void toggleLogToProfile() { setLogToProfile( !isLogToProfile() ); }

    bool isDiscovery() const;
    void setDiscovery( bool state );

    bool sidebarEnabled() const;
    void setSidebarEnabled( bool state );

    int  lastTagType( int type = 0 ) const { return MyQSettings( this ).value( "lasttagtype", type ).toInt(); }
    void setLastTagType( int type )        { MyQSettings( this ).setValue( "lasttagtype", type ); }
    
    int  lastRecommendType()        const { return MyQSettings( this ).value( "lastrecommendtype", 1 ).toInt(); /*Track is default*/ }
    void setLastRecommendType( int type ) { MyQSettings( this ).setValue( "lastrecommendtype", type ); }

    bool resumePlayback() const { return MyQSettings( this ).value( "resumeplayback", 0 ).toInt() == 1; }
    void setResumePlayback( bool enabled );

    StationUrl resumeStation() const
    {
        return StationUrl( MyQSettings( this ).value( "resumestation" ).toString() );
    }
    
    void setResumeStation( StationUrl station );
    
    void addRecentStation( const class Station& );
    void removeRecentStation( int list_index );
    void clearRecentStations( bool emit_signal );
    QList<Station> recentStations();

    QStringList excludedDirs() const;
    void setExcludedDirs( QStringList dirs );
    
    QString bootStrapPluginId() const;
    void setBootStrapPluginId( QString id );

    QStringList includedDirs() const;
    void setIncludedDirs( QStringList dirs );

    bool isMetaDataEnabled();
    void setMetaDataEnabled( bool enabled );

    bool crashReportingEnabled();
    void setCrashReportingEnabled( bool enabled );

    int scrobblePoint();
    void setScrobblePoint( int scrobblePoint );
    
    bool fingerprintingEnabled();
    void setFingerprintingEnabled( bool enabled );

    QString lastRecommendee() const { return MyQSettings( this ).value( "LastRecommendee" ).toString(); }
    void    setLastRecommendee( QString v ) { MyQSettings( this ).setValue( "LastRecommendee", v ); }
    
    int personalTagsListSortOrder() const { return MyQSettings( this ).value( "PersonalTagListSortOrder" ).toInt(); };
    void setPersonalTagsListSortOrder( int sortOrder ) { MyQSettings( this ).setValue( "PersonalTagListSortOrder", sortOrder ); };
    int publicTagsListSortOrder() const { return MyQSettings( this ).value( "PublicTagListSortOrder" ).toInt(); };
    void setPublicTagsListSortOrder( int sortOrder ) { MyQSettings( this ).setValue( "PublicTagListSortOrder", sortOrder ); };

    int sideBarTagsSortOrder() const { return MyQSettings( this ).value( "SideBarTagsSortOrder" ).toInt(); };
    void setSideBarTagsSortOrder( int sortOrder ) { MyQSettings( this ).setValue( "SideBarTagsSortOrder", sortOrder ); };
    int sideBarNeighbourSortOrder() const { return MyQSettings( this ).value( "SideBarNeighbourSortOrder" ).toInt(); };
    void setSideBarNeighbourSortOrder( int sortOrder ) { MyQSettings( this ).setValue( "SideBarNeighbourSortOrder", sortOrder ); };
    
    void setTrackFrameClockMode( bool trackTimeEnabled );
    bool trackFrameClockMode();

    void setLaunchWithMediaPlayer( bool en );
    bool launchWithMediaPlayer();

    void setiPodScrobblingEnabled( bool en );
    bool isiPodScrobblingEnabled();

signals:
    void userChanged( QString username );
    void historyChanged();

private:
};


class MOOSE_DLLEXPORT LastFmSettings : public AppSettings<QSettings>
{
    Q_OBJECT
    
    LastFmSettings( QObject* parent );

    friend LastFmSettings &The::settings();

public:
    QByteArray splitterState() const { return QSettings().value( "splitterState" ).toByteArray(); }
    void setSplitterState( QByteArray state ) { QSettings().setValue( "splitterState", state ); }

    int sidebarWidth() const { return QSettings().value( "sidebarWidth", 190 ).toInt(); }
    void setSidebarWidth( const int width ) { QSettings().setValue( "sidebarWidth", width ); }

    QStringList allUsers() const { return UsersSettings<QSettings>().childGroups(); }
    void setCurrentUsername( QString username );
    QString currentUsername() const { return UsersSettings<QSettings>().value( CURRENT_USER_KEY ).toString(); }    

    LastFmUserSettings& user( QString username ) const;
    LastFmUserSettings& currentUser();

    bool deleteUser( QString username );
    bool isExistingUser( QString username ) const { return UsersSettings<QSettings>().contains( username + "/Password" ); }

    QStringList allPlugins( bool withVersions = true );

    // Returns "" if QSettings().value not found
    QString pluginVersion( QString pluginId );
    QString pluginPlayerPath( QString pluginId );
    void setPluginPlayerPath( QString pluginId, QString path );

    QStringList allMediaDevices();

    /// Returns "" if QSettings().value not found
    QString mediaDeviceUser( QString uid ) const;
    void addMediaDevice( QString uid, QString username );
    void removeMediaDevice( QString uid );

    void setIsManualIpod( QString uid, bool b ) { MediaDeviceSettings().setValue( uid + "/isManualIpod", b ); }
    bool isManualIpod( QString uid ) const { return MediaDeviceSettings().value( uid + "/isManualIpod", false ).toBool(); }

    int  volume()     const { return QSettings().value( "volume", 50 ).toInt(); }
    void setVolume( int v ) { QSettings().setValue( "volume", v ); }

    int  soundCard()     const { return QSettings().value( "soundcard", 0 ).toInt(); }
    void setSoundCard( int v ) { QSettings().setValue( "soundcard", v ); }

    int  soundSystem()     const { return QSettings().value( "soundsystem", 0 ).toInt(); }
    void setSoundSystem( int v ) { QSettings().setValue( "soundsystem", v ); }

    bool isBufferManagedAutomatically() const { return QSettings().value( "BufferManagedAutomatically", 1 ).toBool(); }
    void setBufferManagedAutomatically( bool v ) { QSettings().setValue( "BufferManagedAutomatically", v ); }

    int  httpBufferSize()     const { return QSettings().value( "HttpBufferSize", MooseConstants::kHttpBufferMinSize ).toInt(); }
    void setHttpBufferSize( int v ) { QSettings().setValue( "HttpBufferSize", v ); }

    int externalSoundSystem();

    QString browser()            const { return QSettings().value( "Browser" ).toString(); }
    void setBrowser( QString browser ) { QSettings().setValue( "Browser", browser ); }

    int  musicProxyPort() const { return QSettings().value( "MusicProxyPort" ).toInt(); }
    void setMusicProxyPort( int v ) { QSettings().setValue( "MusicProxyPort", v ); }

    int  controlPort() const { return QSettings().value( "ControlPort", kDefaultControlPort ).toInt(); }
    void setControlPort( int v ) { QSettings().setValue( "ControlPort", v ); }

    int  helperControlPort() const { return QSettings().value( "HelperControlPort", kDefaultHelperControlPort ).toInt(); }
    void setHelperControlPort( int v ) { QSettings().setValue( "HelperControlPort", v ); }

    // This is a string for legacy reasons
    bool isFirstRun() const;
    void setFirstRunDone();

    // This is a string for legacy reasons
    bool isBootstrapDone() const { return QSettings().value( "BootStrapDone", "0" ).toBool(); }
    void setBootstrapDone()      { QSettings().setValue( "BootStrapDone", "1" ); }

    bool  isDontAsk( QString operation_name ) const;
    void setDontAsk( QString operation_name, bool );

    bool showTrayIcon() const { return QSettings().value( "ShowTrayIcon", "1" ).toBool(); }
    void setShowTrayIcon( bool en );

    QString fingerprintUploadUrl() const { return QSettings().value( "FingerprintUploadUrl" ).toString(); }
    void setFingerprintUploadUrl( QString v ) { QSettings().setValue( "FingerprintUploadUrl", v ); }

    MooseEnums::UserIconColour getFreeColour();

signals:
    /// Some property belonging to the current user changed
    void userSettingsChanged( LastFmUserSettings& user );

    /// Some appearance property changed (e.g. tray-icon)
    void appearanceSettingsChanged();

    /// The current user changed to a different user
    void userSwitched( LastFmUserSettings& newUser );

private slots:
    void userChanged( QString username );

private:
    // because we have to return by reference :(
    LastFmUserSettings m_nullUser;
};


class UserQSettings : public UsersSettings<QSettings>
{
public:
    UserQSettings( LastFmUserSettings* user, QObject* parent = 0 ) :
        UsersSettings<QSettings>( parent )
    {
        beginGroup( user->username() );
    }
};


namespace The
{
    inline LastFmSettings &settings()
    {
        //TODO maybe better to have a static instantiate() function
        // thus we lose the need for a mutex

        static QMutex mutex;
        static LastFmSettings* settings = 0;

        QMutexLocker locker( &mutex );

        if (!settings)
        {
            settings = QCoreApplication::instance()->findChild<LastFmSettings*>( "Settings-Instance" );
            if (!settings)
            {
                settings = new LastFmSettings( QCoreApplication::instance() );
                settings->setObjectName( "Settings-Instance" );
            }
        }
        return *settings;
    }

    inline LastFmUserSettings &currentUser()
    {
        return The::settings().currentUser();
    }

    inline QString currentUsername()
    {
        return The::settings().currentUsername();
    }
}


class CurrentUserSettings : public UserQSettings
{
public:
    CurrentUserSettings( QObject* parent = 0 )
            : UserQSettings( &The::currentUser(), parent )
    {}
};

#endif
