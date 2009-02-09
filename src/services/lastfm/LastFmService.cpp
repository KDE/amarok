/***************************************************************************
* copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>          *
* copyright            : (C) 2009 Casey Link <unnamedrambler@gmail.com>   *
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

#include "AvatarDownloader.h"
#include "EngineController.h"
#include "LastFmServiceCollection.h"
#include "LastFmServiceConfig.h"
#include "LoveTrackAction.h"
#include "SimilarArtistsAction.h"
#include "LastFmTreeModel.h"
#include "LastFmTreeView.h"
#include "ScrobblerAdapter.h"
#include "StatusBar.h"
#include "widgets/FlowLayout.h"
#include "GlobalCollectionActions.h"
#include "GlobalCurrentTrackActions.h"
#include "collection/CollectionManager.h"
#include "meta/capabilities/LastFmCapability.h"
#include "meta/LastFmMeta.h"
#include "playlist/PlaylistController.h"
#include "widgets/SearchWidget.h"

#include "kdenetwork/knetworkaccessmanager.h"

#include <lastfm/Scrobbler.h> // from liblastfm
#include <lastfm/ws/WsAccessManager.h>
#include <lastfm/ws/WsKeys.h>
#include <lastfm/ws/WsReply.h>
#include <lastfm/ws/WsRequestBuilder.h>

#include <KLocale>
#include <solid/networking.h>

#include <QComboBox>
#include <QCryptographicHash>
#include <QGroupBox>
#include <QPainter>
#include <QImage>
#include <QFrame>

AMAROK_EXPORT_PLUGIN( LastFmServiceFactory )

QString md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}

void
LastFmServiceFactory::init()
{
#ifdef HAVE_NETWORKMANAGER
    if( Solid::Networking::status() == Solid::Networking::Connected )
    {
        ServiceBase *service = createLastFmService();
        if( service )
        {
            m_activeServices << service;
            m_initialized = true;
            emit newService( service );
        }
    }

        connect( Solid::Networking::notifier(), SIGNAL( shouldConnect() ),
                 this, SLOT( slotCreateLastFmService() ) );
        connect( Solid::Networking::notifier(), SIGNAL( shouldDisconnect() ),
                    this, SLOT( slotRemoveLastFmService() ) );
#else HAVE_NETWORKMANAGER
    ServiceBase *service = createLastFmService();
    if( service )
    {
        m_activeServices << service;
        m_initialized = true;
        emit newService( service );
    }
#endif
}

void
LastFmServiceFactory::slotCreateLastFmService()
{
    if( !m_initialized ) // Until we can remove a service when networking gets disabled, only create it the first time.
    {
        ServiceBase *service = createLastFmService();
        if( service )
        {
            m_activeServices << service;
            m_initialized = true;
            emit newService( service );
        }
    }
}

void
LastFmServiceFactory::slotRemoveLastFmService()
{
    if( m_activeServices.size() == 0 )
        return;

    m_initialized = false;
    emit removeService( m_activeServices.first() );
    m_activeServices.clear();
}

ServiceBase*
LastFmServiceFactory::createLastFmService()
{
    LastFmServiceConfig config;

    //  The user activated the service, but didn't fill the username/password? Don't start it.
    if ( config.username().isEmpty() || config.password().isEmpty() )
        return 0;

    ServiceBase* service = new LastFmService( this, "Last.fm", config.username(), config.password(), config.scrobble(), config.fetchSimilar() );
    return service;
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
    : ServiceBase( name, parent, false ),
      m_scrobble( scrobble ),
      m_scrobbler( 0 ),
      m_polished( false ),
      m_avatarLabel( 0 ),
      m_profile( 0 ),
      m_userinfo( 0 ),
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

    // set up proxy
    WsAccessManager* qnam = new KNetworkAccessManager( this );
    WsRequestBuilder::setWAM( qnam );

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


    //add the "play simmilar artists" action to all artist
    The::globalCollectionActions()->addArtistAction( new SimilarArtistsAction( this ) );
    The::globalCollectionActions()->addTrackAction( new LoveTrackAction( this ) );


    QAction * loveAction = new QAction( KIcon( "love-amarok" ), i18n( "Last.fm: Love" ), this );
    connect( loveAction, SIGNAL( triggered() ), this, SLOT( love() ) );
    loveAction->setShortcut( i18n( "Ctrl+L" ) );
    The::globalCurrentTrackActions()->addAction( loveAction );


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
    try
    {
        switch (reply->error())
        {
            case Ws::NoError:
            {
                m_sessionKey = reply->lfm()["session"]["key"].nonEmptyText();
                Ws::SessionKey = qstrdup( m_sessionKey.toLatin1().data() );
                if( m_scrobble )
                    m_scrobbler = new ScrobblerAdapter( this, "ark" );
                QString sign_key = md5( ( "api_key" + QString( Ws::ApiKey ) + "sk" + QString( Ws::SessionKey ) + "methoduser.getInfo" + QString( Ws::SharedSecret ) ).toUtf8() );
                WsReply* getinfo = WsRequestBuilder( "user.getInfo" )

                .get();

                connect( getinfo, SIGNAL( finished( WsReply* ) ), SLOT( onGetUserInfo( WsReply* ) ) );

                break;
            } case Ws::AuthenticationFailed:
                The::statusBar()->longMessage( i18nc("Last.fm: errorMessage", "Sorry, we don't recognise that username, or you typed the password wrongly." ) );
                break;

            default:
                The::statusBar()->longMessage( i18nc("Last.fm: errorMessage", "There was a problem communicating with the Last.fm services. Please try again later." ) );
                break;

            case Ws::UrProxyIsFuckedLol:
            case Ws::UrLocalNetworkIsFuckedLol:
                The::statusBar()->longMessage( i18nc("Last.fm: errorMessage", "Last.fm cannot be reached. Please check your firewall settings." ) );
                break;
        }
    }
    catch (CoreDomElement::Exception& e)
    {
        qWarning() << "Caught an exception - perhaps the web service didn't reply?" << e;
    }
}

void
LastFmService::onGetUserInfo( WsReply* reply )
{
    DEBUG_BLOCK
    try
    {
        switch (reply->error())
        {
            case Ws::NoError:
            {
                m_country = reply->lfm()["user"]["country"].nonEmptyText();
                m_age = reply->lfm()["user"]["age"].nonEmptyText();
                m_gender = reply->lfm()["user"]["gender"].nonEmptyText();
                m_playcount = reply->lfm()["user"]["playcount"].nonEmptyText();
                m_subscriber = reply->lfm()["user"]["subscriber"].nonEmptyText() == "1";
                debug() << "profile info "  << m_country << " " << m_age << " " << m_gender << " " << m_playcount << " " << m_subscriber;
                if( !reply->lfm()["user"][ "image" ].text().isEmpty() )
                {
                    debug() << "profile avatar: " << reply->lfm()["user"][ "image" ].text();
                    AvatarDownloader* downloader = new AvatarDownloader();
                    KUrl url( reply->lfm()["user"][ "image" ].text() );
                    downloader->downloadAvatar( m_userName,  url);
                    connect( downloader, SIGNAL( signalAvatarDownloaded( QPixmap ) ), SLOT( onAvatarDownloaded( QPixmap ) ) );
                }
                updateProfileInfo();
                break;
            } case Ws::AuthenticationFailed:
//             debug() << "Last.fm: errorMessage", "%1: %2", "Last.fm", "Sorry, we don't recognise that username, or you typed the password wrongly.";
            break;

            default:
//                 debug() << "Last.fm: errorMessage", "%1: %2", "Last.fm", "There was a problem communicating with the Last.fm services. Please try again later.";
                break;

            case Ws::UrProxyIsFuckedLol:
            case Ws::UrLocalNetworkIsFuckedLol:
//                 debug() <<  "Last.fm: errorMessage", "%1: %2", "Last.fm", "Last.fm cannot be reached. Please check your firewall settings.";
                break;
        }
    }
    catch (CoreDomElement::Exception& e)
    {
        qWarning() << "Caught an exception - perhaps the web service didn't reply?" << e;
    }
}

void
LastFmService::onAvatarDownloaded( QPixmap avatar )
{
    DEBUG_BLOCK
    if( !avatar.isNull() ) {
        int m = 48;
        avatar = avatar.scaled( m, m, Qt::KeepAspectRatio, Qt::SmoothTransformation );

        // This code is here to stop Qt from crashing on certain weirdly shaped avatars.
        // We had a case were an avatar got a height of 1px after scaling and it would
        // crash in the rendering code. This here just fills in the background with
        // transparency first.
        if ( avatar.width() < m || avatar.height() < m )
        {
            QImage finalAvatar( m, m, QImage::Format_ARGB32 );
            finalAvatar.fill( 0 );

            QPainter p( &finalAvatar );
            QRect r;

            if ( avatar.width() < m )
                r = QRect( ( m - avatar.width() ) / 2, 0, avatar.width(), avatar.height() );
            else
                r = QRect( 0, ( m - avatar.height() ) / 2, avatar.width(), avatar.height() );

            p.drawPixmap( r, avatar );
            p.end();

            avatar = QPixmap::fromImage( finalAvatar );
        }
        m_avatar = avatar;
        if( m_avatarLabel )
            m_avatarLabel->setPixmap( m_avatar );
    }
//     sender()->deleteLater();
}

void
LastFmService::updateEditHint( int index )
{
    if( !m_customStationEdit )
        return;
    QString hint;
    switch ( index ) {
        case 0:
            hint = i18n( "Enter an artist name" );
            break;
        case 1:
            hint = i18n( "Enter a tag" );
            break;
        case 2:
            hint = i18n( "Enter a last.fm user name" );
            break;
        default:
            return;
    }
    m_customStationEdit->setClickMessage( hint );
}

void
LastFmService::updateProfileInfo()
{
    if( m_userinfo && !m_age.isEmpty() && !m_gender.isEmpty()) {
        QString ageinfo = " (" + m_age + ", " + m_gender + ')';
        m_userinfo->setText( m_userName + ageinfo);
    }
    if( m_profile && !m_playcount.isEmpty() ) {
        QString playcount = KGlobal::locale()->formatNumber( m_playcount, false );
        m_profile->setText( playcount + i18n( " plays" ) );
    }
}

void
LastFmService::polish()
{
    if( !m_polished )
    {
        LastFmTreeView* view = new LastFmTreeView( this );
        view->setFrameShape( QFrame::NoFrame );
        view->setDragEnabled ( true );
        view->setSortingEnabled( false );
        view->setDragDropMode ( QAbstractItemView::DragOnly );
        setView( view );
        setModel( new LastFmTreeModel( m_userName, this ) );

        m_bottomPanel->setMaximumHeight( 300 );
//         m_bottomPanel->hide();
        m_buttonBox = new QWidget( m_bottomPanel );
        FlowLayout * flowLayout = new FlowLayout( 3 );
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

        m_topPanel->setMaximumHeight( 300 );
        KHBox * outerProfilebox = new KHBox( m_topPanel );
        outerProfilebox->setSpacing(1);
        outerProfilebox->setMargin(0);

        m_avatarLabel = new QLabel(outerProfilebox);
        if( !m_avatar )
        {
            m_avatarLabel->setPixmap( KIcon( "filename-artist-amarok" ).pixmap(32, 32) );
            m_avatarLabel->setFixedSize( 32, 32 );
        }
        else
        {
            m_avatarLabel->setPixmap( m_avatar );
            m_avatarLabel->setFixedSize( m_avatar.width(), m_avatar.height() );
        }


        debug() << m_avatarLabel->margin();
        KVBox * innerProfilebox = new KVBox( outerProfilebox );
        innerProfilebox->setSpacing(1);
        innerProfilebox->setMargin(0);
        m_userinfo = new QLabel(innerProfilebox);
        m_userinfo->setText( m_userName );
        m_userinfo->setAlignment( Qt::AlignCenter | Qt::AlignHCenter );
        m_userinfo->setMinimumSize( 230 , 28 );
        m_profile = new QLabel(innerProfilebox);
        m_profile->setText(QString());
        m_profile->setAlignment( Qt::AlignCenter | Qt::AlignHCenter );
        updateProfileInfo();

        QGroupBox *customStation = new QGroupBox( i18n( "Create a Custom Last.fm Station" ), m_topPanel );
        m_customStationCombo = new QComboBox;
        QStringList choices;
        choices << i18n( "Artist" ) << i18n( "Tag" ) << i18n( "User" );
        m_customStationCombo->insertItems(0, choices);
        m_customStationEdit = new KLineEdit;
        updateEditHint( m_customStationCombo->currentIndex() );
        m_customStationButton = new QPushButton;
        m_customStationButton->setObjectName( "customButton" );
        m_customStationButton->setIcon( KIcon( "media-playback-start-amarok" ) );
        QHBoxLayout *hbox = new QHBoxLayout();
        hbox->addWidget(m_customStationCombo);
        hbox->addWidget(m_customStationEdit);
        hbox->addWidget(m_customStationButton);
        customStation->setLayout(hbox);

        connect( m_customStationEdit, SIGNAL( returnPressed() ), this, SLOT( playCustomStation() ) );
        connect( m_customStationButton, SIGNAL( clicked() ), this, SLOT( playCustomStation() ) );
        connect( m_customStationCombo, SIGNAL( currentIndexChanged(int) ), this, SLOT( updateEditHint(int) ));

        QList<int> levels;
        levels << CategoryId::Genre << CategoryId::Album;
        m_polished = true;
    }
}


void
LastFmService::love()
{
    DEBUG_BLOCK

    Meta::TrackPtr track = The::engineController()->currentTrack();
    LastFm::Track* lastfmTrack = dynamic_cast< LastFm::Track* >( track.data() );
    if( track )
        The::statusBar()->shortMessage( i18nc( "As in, lastfm", "Loved Track: %1", track->prettyName() ) );

    if( lastfmTrack )
        lastfmTrack->love();
    else
        m_scrobbler->loveTrack( track );

}

void LastFmService::love( Meta::TrackPtr track )
{
    DEBUG_BLOCK
    if( track )
        The::statusBar()->shortMessage( i18nc( "As in, lastfm", "Loved Track: %1", track->prettyName() ) );
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
    DEBUG_BLOCK
    QString text = m_customStationEdit->text();
    QString station;
    debug() << "Selected combo " <<m_customStationCombo->currentIndex();
    switch ( m_customStationCombo->currentIndex() ) {
        case 0:
            station = "lastfm://artist/" + text + "/similarartists";
            break;
        case 1:
            station = "lastfm://globaltags/" + text;
            break;
        case 2:
            station = "lastfm://user/" + text + "/personal";
            break;
        default:
            return;
    }

    if ( !station.isEmpty() ) {
        playLastFmStation( station );
    }
}

void LastFmService::playLastFmStation( const KUrl &url )
{
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
    The::playlistController()->insertOptioned( track, Playlist::AppendAndPlay );
}

Amarok::Collection * LastFmService::collection()
{
    return m_collection;
}



