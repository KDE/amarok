/***************************************************************************
* copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>          *
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
#include "SimilarArtistsAction.h"
#include "ScrobblerAdapter.h"
#include "StatusBar.h"
#include "widgets/FlowLayout.h"

#include "GlobalCollectionActions.h"

#include "collection/CollectionManager.h"
#include "meta/LastFmCapability.h"
#include "meta/LastFmMeta.h"
#include "playlist/PlaylistController.h"
#include "widgets/SearchWidget.h"

#include <lastfm/Scrobbler.h> // from liblastfm
#include <lastfm/ws/WsKeys.h>
#include <lastfm/ws/WsReply.h>
#include <lastfm/ws/WsRequestBuilder.h>

#include <QComboBox>
#include <QCryptographicHash>

AMAROK_EXPORT_PLUGIN( LastFmServiceFactory )

QString md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}

void
LastFmServiceFactory::init()
{
    LastFmServiceConfig config;
 
    //  The user activated the service, but didn't fill the username/password? Don't start it.
    if ( config.username().isEmpty() || config.password().isEmpty() ) return;
    
    ServiceBase* service = new LastFmService( this, "Last.fm", config.username(), config.password(), config.scrobble(), config.fetchSimilar() );
    m_activeServices << service;
    m_initialized = true;
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
      m_scrobble( scrobble ),
      m_scrobbler( 0 ),
      m_polished( false ),
      m_userName( username )
{
    DEBUG_BLOCK
    
    Q_UNUSED( fetchSimilar ); // TODO implement..
    
    // set the global static Lastfm::Ws stuff
    Ws::ApiKey = "402d3ca8e9bc9d3cf9b85e1202944ca5";
    Ws::SharedSecret = "fe0dcde9fcd14c2d1d50665b646335e9";
    // testing w/ official keys
    //Ws::SharedSecret = "73582dfc9e556d307aead069af110ab8";
    //Ws::ApiKey = "c8c7b163b11f92ef2d33ba6cd3c2c3c3";
    Ws::Username = qstrdup( m_userName.toLatin1().data() );
    
    debug() << "username:" << QString( QUrl::toPercentEncoding( Ws::Username ) );

    QString authToken =  md5( ( m_userName + md5( password.toUtf8() ) ).toUtf8() );
    QString sign_key = md5( ( "api_key" + QString( Ws::ApiKey ) + "authToken" + authToken + "methodauth.getMobileSession" + QString( Ws::SharedSecret ) ).toUtf8() );
    
    // now authenticate w/ last.fm and get our session key
    WsReply* reply = WsRequestBuilder( "auth.getMobileSession" )
    .add( "username", m_userName )
    .add( "authToken", authToken )
    .add( "api_key", Ws::ApiKey )
    .add( "api_sig", sign_key )
    .get();
    
    connect( reply, SIGNAL( finished( WsReply* ) ), SLOT( onAuthenticated( WsReply* ) ) );
    
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
}


LastFmService::~LastFmService()
{
    DEBUG_BLOCK

    CollectionManager::instance()->removeUnmanagedCollection( m_collection );
    ms_service = 0;
    delete m_collection;
}

void
LastFmService::onAuthenticated( WsReply* reply )
{
    switch (reply->error())
    {
        case Ws::NoError:
        {
            m_sessionKey = reply->lfm()["session"]["key"].nonEmptyText();
            Ws::SessionKey = qstrdup( m_sessionKey.toLatin1().data() );
            if( m_scrobble )
                m_scrobbler = new ScrobblerAdapter( this, "ark" );
            break;
        } case Ws::AuthenticationFailed:
            //The::statusBar()->longMessage( i18nc("Last.fm: errorMessage", "%1: %2", "Last.fm", "Sorry, we don't recognise that username, or you typed the password wrongly." ), KDE::StatusBar::Error );
            break;
            
        default:
            //The::statusBar()->longMessage( i18nc("Last.fm: errorMessage", "%1: %2", "Last.fm", "There was a problem communicating with the Last.fm services. Please try again later." ), KDE::StatusBar::Error );
            break;
            
        case Ws::UrProxyIsFuckedLol:
        case Ws::UrLocalNetworkIsFuckedLol:
            //The::statusBar()->longMessage( i18nc("Last.fm: errorMessage", "%1: %2", "Last.fm", "Last.fm cannot be reached. Please check your firewall settings." ), KDE::StatusBar::Error );
            break;
    }
}

void
LastFmService::polish()
{
    if( !m_polished )
    {
        m_bottomPanel->setMaximumHeight( 150 );
        m_buttonBox = new QWidget( m_bottomPanel );
        FlowLayout * flowLayout= new FlowLayout( 3 );
        m_buttonBox->setLayout( flowLayout );
        
        m_loveButton = new QPushButton( );
        m_loveButton->setText( i18n( "Love" ) );
        m_loveButton->setObjectName( "loveButton" );
        m_loveButton->setIcon( KIcon( "love-amarok" ) );
        connect( m_loveButton, SIGNAL( clicked() ), this, SLOT( love() ) );
        flowLayout->addWidget( m_loveButton );

        m_banButton = new QPushButton();
        m_banButton->setText( i18n( "Ban" ) );
        m_banButton->setObjectName( "banButton" );
        m_banButton->setIcon( KIcon( "remove-amarok" ) );
        connect( m_banButton, SIGNAL( clicked() ), this, SLOT( ban() ) );
        flowLayout->addWidget( m_banButton );

        m_skipButton = new QPushButton();
        m_skipButton->setText( i18n( "Skip" ) );
        m_skipButton->setObjectName( "skipButton" );
        m_skipButton->setIcon( KIcon( "media-seek-forward-amarok" ) );
        connect( m_skipButton, SIGNAL( clicked() ), this, SLOT( skip() ) );
        flowLayout->addWidget( m_skipButton );

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

    Meta::TrackPtr track = The::engineController()->currentTrack();
    LastFm::Track* lastfmTrack = dynamic_cast< LastFm::Track* >( track.data() );
    if( lastfmTrack )
        lastfmTrack->love();
    else
        m_scrobbler->loveTrack( track );

}


void
LastFmService::ban()
{
    DEBUG_BLOCK
    
    Meta::TrackPtr track = The::engineController()->currentTrack();
    LastFm::Track* lastfmTrack = dynamic_cast< LastFm::Track* >( track.data() );
    if( lastfmTrack )
        lastfmTrack->ban();
}


void
LastFmService::skip()
{
    DEBUG_BLOCK
    
    Meta::TrackPtr track = The::engineController()->currentTrack();
    LastFm::Track* lastfmTrack = dynamic_cast< LastFm::Track* >( track.data() );
    if( lastfmTrack )
        lastfmTrack->skip();
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
