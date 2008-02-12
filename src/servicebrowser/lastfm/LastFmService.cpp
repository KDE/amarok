/***************************************************************************
 * copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "LastFmService.h"
#include "LastFmServiceCollection.h"
#include "LastFmServiceConfig.h"
#include "RadioAdapter.h"
#include "ScrobblerAdapter.h"

#include "CollectionManager.h"
#include "meta/LastFmCapability.h"
#include "playlist/PlaylistModel.h"
#include "TheInstances.h"


AMAROK_EXPORT_PLUGIN( LastFmServiceFactory )


void 
LastFmServiceFactory::init()
{
    LastFmServiceConfig config;

    ServiceBase* service = new LastFmService( "Last.fm", config.username(), UnicornUtils::md5Digest( config.password().toUtf8() ), config.scrobble(), config.fetchSimilar() );
    emit newService( service );
}


QString 
LastFmServiceFactory::name()
{
    return "Last.fm";
}


KPluginInfo 
LastFmServiceFactory::info()
{
    KPluginInfo pluginInfo(  "amarok_service_lastfm.desktop", "services" );
    pluginInfo.setConfig( config() );
    return pluginInfo;
}


KConfigGroup 
LastFmServiceFactory::config()
{
    return Amarok::config( LastFmServiceConfig::configSectionName() );
}


LastFmService::LastFmService( const QString &name, const QString &username, const QString &password, bool scrobble, bool fetchSimilar )
    : ServiceBase( name ),
      m_scrobbler( scrobble ? new ScrobblerAdapter( this, username, password ) : 0 ),
      m_radio( new RadioAdapter( this, username, password ) ),
      m_collection( new LastFmServiceCollection( ) ),
      m_polished( false )
{
    setShortDescription(  i18n( "Last.fm: The social music revolution." ) );
    setIcon( KIcon( "view-services-lastfm-amarok" ) );
    showInfo( false );

    Q_ASSERT( ms_service == 0 );
    ms_service = this;
}


LastFmService::~LastFmService()
{
    ms_service = 0;

    CollectionManager::instance()->removeUnmanagedCollection( m_collection );
    delete m_collection;
}


void
LastFmService::polish()
{
    if( !m_polished )
    {
        m_bottomPanel->setMaximumHeight( 100 );
        
        
        m_buttonBox = new KHBox(m_bottomPanel);
        m_buttonBox->setSpacing( 3 );

        m_loveButton = new QPushButton( m_buttonBox );
        m_loveButton->setText( i18n( "Love" ) );
        m_loveButton->setObjectName( "loveButton" );
        m_loveButton->setIcon( KIcon( "amarok_love" ) );
        connect( m_loveButton, SIGNAL( clicked() ), this, SLOT( love() ) );

        m_banButton = new QPushButton( m_buttonBox );
        m_banButton->setText( i18n( "Ban" ) );
        m_banButton->setObjectName( "banButton" );
        m_banButton->setIcon( KIcon( "amarok_remove" ) );
        connect( m_banButton, SIGNAL( clicked() ), this, SLOT( ban() ) );

        m_skipButton = new QPushButton( m_buttonBox );
        m_skipButton->setText( i18n( "Skip" ) );
        m_skipButton->setObjectName( "skipButton" );
        m_skipButton->setIcon( KIcon( "media-seek-forward-amarok" ) );
        connect( m_skipButton, SIGNAL( clicked() ), this, SLOT( skip() ) );

        connect( m_radio, SIGNAL( haveTrack( bool ) ), this, SLOT( setButtonsEnabled( bool ) ) );

        setButtonsEnabled( m_radio->currentTrack() );

        KHBox * customStationBox = new KHBox( m_bottomPanel );
        customStationBox->setSpacing( 3 );
        m_customStationEdit = new KLineEdit( customStationBox );
        m_customStationEdit->setClickMessage( i18n( "Enter artist name" ) );
        m_customStationButton = new QPushButton( customStationBox );
        m_customStationButton->setText( i18n( "Go" ) );
        m_customStationButton->setObjectName( "customButton" );
        m_customStationButton->setIcon( KIcon( "media-playback-start-amarok" ) );

        connect( m_customStationButton, SIGNAL( clicked() ), this, SLOT( playCustomStation() ) );
        
        m_polished = true;
    }
}


void
LastFmService::love()
{
    LastFm::TrackPtr track = radio()->currentTrack();
    if( track )
        track->love();
}


void
LastFmService::ban()
{
    LastFm::TrackPtr track = radio()->currentTrack();
    if( track )
        track->ban();
}


void
LastFmService::skip()
{
    LastFm::TrackPtr track = radio()->currentTrack();
    if( track )
        track->skip();
}


void
LastFmService::setButtonsEnabled( bool enable )
{
    m_buttonBox->setEnabled( enable );
}


LastFmService *LastFmService::ms_service = 0;


namespace The
{
    LastFmService *lastFmService()
    {
        return LastFmService::ms_service;
    }
}

void LastFmService::playCustomStation()
{
    DEBUG_BLOCK
    QString band = m_customStationEdit->text();

    if ( !band.isEmpty() ) {
        QString url = "lastfm://artist/" + band + "/similarartists";
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl( url ) );
        The::playlistModel()->insertOptioned( track, Playlist::Append|Playlist::DirectPlay );
    }
}
