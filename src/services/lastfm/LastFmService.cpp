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

#include "EngineController.h"
#include "LastFmServiceCollection.h"
#include "LastFmServiceConfig.h"
#include "RadioAdapter.h"
#include "ScrobblerAdapter.h"
#include "SimilarArtistsAction.h"

#include "GlobalCollectionActions.h"

#include "collection/CollectionManager.h"
#include "meta/LastFmCapability.h"
#include "playlist/PlaylistController.h"
#include "widgets/SearchWidget.h"

#include <QComboBox>

AMAROK_EXPORT_PLUGIN( LastFmServiceFactory )

void
LastFmServiceFactory::init()
{
    LastFmServiceConfig config;
 
    //  The user activated the service, but didn't fill the username/password? Don't start it.
    if ( config.username().isEmpty() || config.password().isEmpty() ) return; 

    ServiceBase* service = new LastFmService( this, "Last.fm", config.username(), UnicornUtils::md5Digest( config.password().toUtf8() ), config.scrobble(), config.fetchSimilar() );
    m_activeServices << service;
    connect( service, SIGNAL( ready() ), this, SLOT( serviceReady() ) );
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


LastFmService::LastFmService( LastFmServiceFactory* parent, const QString &name, const QString &username, const QString &password, bool scrobble, bool fetchSimilar )
    : ServiceBase( name, parent ),
      m_scrobbler( scrobble ? new ScrobblerAdapter( this, username, password ) : 0 ),
      m_radio( new RadioAdapter( this, username, password ) ),
      m_polished( false ),
      m_userName( username )
{
    Q_UNUSED( fetchSimilar ); // TODO implement..
    //We have no use for searching currently..
    m_searchWidget->setVisible( false );
    setShortDescription( i18n( "Last.fm: The social music revolution." ) );
    setIcon( KIcon( "view-services-lastfm-amarok" ) );

    m_collection = new LastFmServiceCollection( m_userName );
    CollectionManager::instance()->addUnmanagedCollection( m_collection, CollectionManager::CollectionDisabled );


    //add the "plas simmilar artists" action to all artist
    The::globalCollectionActions()->addArtistAction( new SimilarArtistsAction( this ) );

    Q_ASSERT( ms_service == 0 );
    ms_service = this;
    m_serviceready = true;
    emit( ready() );
}


LastFmService::~LastFmService()
{
    DEBUG_BLOCK

    The::engineController()->stop( true ); //Needed to prevent libunicorn crashing when unloading

    CollectionManager::instance()->removeUnmanagedCollection( m_collection );
    ms_service = 0;
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
        m_loveButton->setIcon( KIcon( "emblem-favorite-amarok" ) );
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

        connect( m_radio, SIGNAL( haveTrack( bool ) ), this, SLOT( setRadioButtons( bool ) ) );
        setRadioButtons( m_radio->currentTrack() );

        KHBox * customStationBox = new KHBox( m_bottomPanel );
        customStationBox->setSpacing( 3 );
        m_customStationEdit = new KLineEdit( customStationBox );
        m_customStationEdit->setClickMessage( i18n( "Enter artist name" ) );
        m_customStationButton = new QPushButton( customStationBox );
        m_customStationButton->setText( i18n( "Go" ) );
        m_customStationButton->setObjectName( "customButton" );
        m_customStationButton->setIcon( KIcon( "media-playback-start-amarok" ) );

        connect( m_customStationEdit, SIGNAL( returnPressed() ), this, SLOT( playCustomStation() ) );
        connect( m_customStationButton, SIGNAL( clicked() ), this, SLOT( playCustomStation() ) );

        QList<int> levels;
        levels << CategoryId::Genre;
        setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );

        m_polished = true;
    }
}


void
LastFmService::love()
{
    DEBUG_BLOCK

    LastFm::TrackPtr radioTrack = radio()->currentTrack();
    if( radioTrack )
        radioTrack->love();

    // We're loving a track which isn't from radio
    else
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if( track )
            m_scrobbler->loveTrack( track );
    }
}


void
LastFmService::ban()
{
    DEBUG_BLOCK

    LastFm::TrackPtr track = radio()->currentTrack();
    if( track )
        track->ban();
}


void
LastFmService::skip()
{
    DEBUG_BLOCK

    LastFm::TrackPtr track = radio()->currentTrack();
    if( track )
        track->skip();
}


void
LastFmService::setRadioButtons( bool hasRadio )
{
    m_loveButton->setEnabled( true ); // we can love any track, anytime
    m_skipButton->setEnabled( hasRadio );
    m_banButton->setEnabled( hasRadio );
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
    QString band = m_customStationEdit->text();

    if ( !band.isEmpty() ) {
        playLastFmStation( "lastfm://artist/" + band + "/similarartists" );
    }
}

void LastFmService::playLastFmStation( const KUrl &url )
{
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
    The::playlistController()->insertOptioned( track, Playlist::AppendAndPlay );
}

Collection * LastFmService::collection()
{
    return m_collection;
}
