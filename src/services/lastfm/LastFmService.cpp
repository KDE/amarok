/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "LastFmService.h"

#include "AvatarDownloader.h"
#include "EngineController.h"
#include "biases/LastFmBias.h"
#include "biases/WeeklyTopBias.h"
#include "browsers/CollectionTreeItem.h"
#include "browsers/CollectionTreeItemModelBase.h"
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
#include "playlist/PlaylistModelStack.h"
#include "widgets/SearchWidget.h"
#include "CustomBias.h"

#include "kdenetwork/knetworkaccessmanager.h"

#include <lastfm/Audioscrobbler> // from liblastfm
#include <lastfm/NetworkAccessManager>
#include <lastfm/XmlQuery>

#include <KLocale>
#include <KPasswordDialog>
#include <KStandardDirs>
#include <solid/networking.h>

#include <QComboBox>
#include <QCryptographicHash>
#include <QGroupBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPainter>
#include <QImage>
#include <QFrame>
#include <QTextDocument>        //Qt::escape

AMAROK_EXPORT_PLUGIN( LastFmServiceFactory )

QString md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}

void
LastFmServiceFactory::init()
{
   /* Fancy network detection is nice, but buggy if you're stepping outside your currently selected
    * backend -- and since this is currently the only service using it, it makes it seem like there's just
    * a last.fm bug.  Disable until such a time as *all* the Internet services go away and are replaced by
    * helpful text describing how to change your backend if your network is actually running.
    */
   // if( Solid::Networking::status() == Solid::Networking::Unknown ) // No working solid network backend, so force creation of the service
    //{
        ServiceBase *service = createLastFmService();
        if( service )
        {
            m_activeServices << service;
            m_initialized = true;
            emit newService( service );
        }
   /* }
    else
    {
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
    } */
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
//     if ( config.username().isEmpty() || config.password().isEmpty() )
//         return 0;

    ServiceBase* service = new LastFmService( this, "Last.fm", config.username(), config.password(), config.sessionKey(), config.scrobble(), config.fetchSimilar() );
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


LastFmService::LastFmService( LastFmServiceFactory* parent, const QString &name, const QString &username, QString password, const QString& sessionKey, bool scrobble, bool fetchSimilar )
    : ServiceBase( name, parent, false ),
      m_inited( false),
      m_scrobble( scrobble ),
      m_scrobbler( 0 ),
      m_collection( 0 ),
      m_polished( false ),
      m_avatarLabel( 0 ),
      m_profile( 0 ),
      m_userinfo( 0 ),
      m_userName( username ),
      m_sessionKey( sessionKey ),
      m_userNameArray( 0 ),
      m_sessionKeyArray( 0 ),
      m_lastFmBiasFactory( 0 ),
      m_weeklyTopBiasFactory( 0 )
{
    DEBUG_BLOCK

    Q_UNUSED( sessionKey );
    Q_UNUSED( fetchSimilar ); // TODO implement..

    setShortDescription( i18n( "Last.fm: The social music revolution" ) );
    setIcon( KIcon( "view-services-lastfm-amarok" ) );
    setLongDescription( i18n( "Last.fm is a popular online service that provides personal radio stations and music recommendations. A personal listening station is tailored based on your listening habits and provides you with recommendations for new music. It is also possible to play stations with music that is similar to a particular artist as well as listen to streams from people you have added as friends or that last.fm considers your musical \"neighbors\"" ) );
    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_lastfm.png" ) );

    if( !username.isEmpty() && !password.isEmpty() )
        init();

}


LastFmService::~LastFmService()
{
    DEBUG_BLOCK

    delete m_lastFmBiasFactory;
    delete m_weeklyTopBiasFactory;
    delete[] m_userNameArray;
    delete[] m_sessionKeyArray;

    if( m_collection && CollectionManager::instance() )
    {
        CollectionManager::instance()->removeUnmanagedCollection( m_collection );
        delete m_collection;
        m_collection = 0;
    }
    ms_service = 0;
}

void
LastFmService::init()
{
    LastFmServiceConfig config;
    const QString password = config.password();
    const QString sessionKey = config.sessionKey();
    // set the global static Lastfm::Ws stuff
    lastfm::ws::ApiKey = Amarok::lastfmApiKey();
    lastfm::ws::SharedSecret = "fe0dcde9fcd14c2d1d50665b646335e9";
    // testing w/ official keys
    //Ws::SharedSecret = "73582dfc9e556d307aead069af110ab8";
    //Ws::ApiKey = "c8c7b163b11f92ef2d33ba6cd3c2c3c3";
    m_userNameArray = qstrdup( m_userName.toLatin1().data() );
    lastfm::ws::Username = m_userNameArray;


    // set up proxy
    QNetworkAccessManager* qnam = new KNetworkAccessManager( this );
    lastfm::setNetworkAccessManager( qnam );

    debug() << "username:" << QString( QUrl::toPercentEncoding( lastfm::ws::Username ) );

    QString authToken =  md5( ( m_userName + md5( password.toUtf8() ) ).toUtf8() );

    // now authenticate w/ last.fm and get our session key if we don't have one
    if( sessionKey.isEmpty() )
    {
        debug() << "got no saved session key, authenticating with last.fm";
        QMap<QString, QString> query;
        query[ "method" ] = "auth.getMobileSession";
        query[ "username" ] = m_userName;
        query[ "authToken" ] = authToken;
        m_jobs[ "auth" ] = lastfm::ws::post( query );

        connect( m_jobs[ "auth" ], SIGNAL( finished() ), SLOT( onAuthenticated() ) );

    } else
    {
        debug() << "using saved sessionkey from last.fm";
        m_sessionKeyArray = qstrdup( sessionKey.toLatin1().data() );
        lastfm::ws::SessionKey = m_sessionKeyArray;
        m_sessionKey = sessionKey;

        if( m_scrobble )
            m_scrobbler = new ScrobblerAdapter( this, "ark" );
        QMap< QString, QString > params;
        params[ "method" ] = "user.getInfo";
        m_jobs[ "getUserInfo" ] = lastfm::ws::post( params );

        connect( m_jobs[ "getUserInfo" ], SIGNAL( finished() ), SLOT( onGetUserInfo() ) );
    }


    //We have no use for searching currently..
    m_searchWidget->setVisible( false );

    // enable custom bias
    m_lastFmBiasFactory = new Dynamic::LastFmBiasFactory();
    Dynamic::CustomBias::registerNewBiasFactory( m_lastFmBiasFactory );

    m_weeklyTopBiasFactory = new Dynamic::WeeklyTopBiasFactory();
    Dynamic::CustomBias::registerNewBiasFactory( m_weeklyTopBiasFactory );

    m_collection = new LastFmServiceCollection( m_userName );
    CollectionManager::instance()->addUnmanagedCollection( m_collection, CollectionManager::CollectionDisabled );


    //add the "play similar artists" action to all artist
    The::globalCollectionActions()->addArtistAction( new SimilarArtistsAction( this ) );
    The::globalCollectionActions()->addTrackAction( new LoveTrackAction( this ) );


    QAction * loveAction = new QAction( KIcon( "love-amarok" ), i18n( "Last.fm: Love" ), this );
    connect( loveAction, SIGNAL( triggered() ), this, SLOT( love() ) );
    loveAction->setShortcut( i18n( "Ctrl+L" ) );
    The::globalCurrentTrackActions()->addAction( loveAction );


    Q_ASSERT( ms_service == 0 );
    ms_service = this;
    m_serviceready = true;

    m_inited = true;
}


void
LastFmService::onAuthenticated()
{
    if( !m_jobs[ "auth" ] )
    {
        debug() << "WARNING: GOT RESULT but no object";
        return;
    }

    switch ( m_jobs[ "auth" ]->error() )
    {
        case QNetworkReply::NoError:
        {

            lastfm::XmlQuery lfm = lastfm::XmlQuery( m_jobs[ "auth" ]->readAll() );
            LastFmServiceConfig config;

            if( lfm.children( "error" ).size() > 0 )
            {
                debug() << "error from authenticating with last.fm service:" << lfm.text();
                config.clearSessionKey();
                break;
            }
            m_sessionKey = lfm[ "session" ][ "key" ].text();

            m_sessionKeyArray = qstrdup( m_sessionKey.toLatin1().data() );
            lastfm::ws::SessionKey = m_sessionKeyArray;
            config.setSessionKey( m_sessionKey );
            config.save();

            if( m_scrobble )
                m_scrobbler = new ScrobblerAdapter( this, "ark" );
            QMap< QString, QString > params;
            params[ "method" ] = "user.getInfo";
            m_jobs[ "getUserInfo" ] = lastfm::ws::post( params );

            connect( m_jobs[ "getUserInfo" ], SIGNAL( finished() ), SLOT( onGetUserInfo() ) );

            break;
        } case QNetworkReply::AuthenticationRequiredError:
            The::statusBar()->longMessage( i18nc("Last.fm: errorMessage", "Either the username was not recognized, or the password was incorrect." ) );
            break;

        default:
            The::statusBar()->longMessage( i18nc("Last.fm: errorMessage", "There was a problem communicating with the Last.fm services. Please try again later." ) );
            break;
    }
    m_jobs[ "auth" ]->deleteLater();
}

void
LastFmService::onGetUserInfo()
{
    DEBUG_BLOCK
    if( !m_jobs[ "getUserInfo" ] )
    {
        debug() << "GOT RESULT FROM USER QUERY, but no object..!";
        return;
    }
    switch (m_jobs[ "getUserInfo" ]->error())
    {
        case QNetworkReply::NoError:
        {
            try
            {
                lastfm::XmlQuery lfm( m_jobs[ "getUserInfo" ]->readAll() );

                m_country = lfm["user"]["country"].text();
                m_age = lfm["user"]["age"].text();
                m_gender = lfm["user"]["gender"].text();
                m_playcount = lfm["user"]["playcount"].text();
                m_subscriber = lfm["user"]["subscriber"].text() == "1";

                debug() << "profile info "  << m_country << " " << m_age << " " << m_gender << " " << m_playcount << " " << m_subscriber;
                if( !lfm["user"][ "image" ].text().isEmpty() )
                {
                    debug() << "profile avatar: " <<lfm["user"][ "image" ].text();
                    AvatarDownloader* downloader = new AvatarDownloader();
                    KUrl url( lfm["user"][ "image" ].text() );
                    downloader->downloadAvatar( m_userName,  url);
                    connect( downloader, SIGNAL( signalAvatarDownloaded( QPixmap ) ), SLOT( onAvatarDownloaded( QPixmap ) ) );
                }
                updateProfileInfo();

            } catch( lastfm::ws::ParseError& e )
            {
                debug() << "Got exception in parsing from last.fm:" << e.what();
            }
            break;
        } case QNetworkReply::AuthenticationRequiredError:
            debug() << "Last.fm: errorMessage: Sorry, we don't recognise that username, or you typed the password incorrectly.";
            break;

        default:
            debug() << "Last.fm: errorMessage: There was a problem communicating with the Last.fm services. Please try again later.";
            break;
    }

    m_jobs[ "getUserInfo" ]->deleteLater();
}

void
LastFmService::onAvatarDownloaded( QPixmap avatar )
{
    DEBUG_BLOCK
    if( !avatar.isNull() ) {

        if( !m_polished )
            polish();

        LastFmTreeModel* lfm = dynamic_cast<LastFmTreeModel*>( model() );

        int m = lfm->avatarSize();
        avatar = avatar.scaled( m, m, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        lfm->prepareAvatar( avatar, m );
        m_avatar = avatar;

        if( m_avatarLabel )
            m_avatarLabel->setPixmap( m_avatar );
    }
    sender()->deleteLater();
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
    if( m_userinfo )
    {
        m_userinfo->setText( i18n( "Username: ") + Qt::escape( m_userName ) );
    }

    if( m_profile && !m_playcount.isEmpty() )
    {
        m_profile->setText( i18np( "Play Count: %1 play", "Play Count: %1 plays", m_playcount.toInt() ) );
    }
}

void
LastFmService::polish()
{
    if( !m_inited )
    {
        KPasswordDialog dlg( 0 , KPasswordDialog::ShowUsernameLine );
        dlg.setPrompt( i18n( "Enter login information for Last.fm" ) );
        if( !dlg.exec() )
            return; //the user canceled

            m_userName = dlg.username();
        const QString password = dlg.password();
        if( password.isEmpty() || m_userName.isEmpty() )
            return; // We can't create the service if we don't get the details..
        LastFmServiceConfig config;
        config.setPassword( password );
        config.setUsername( m_userName );
        config.save();
        init();
    }
    if( !m_polished )
    {
        LastFmTreeView* view = new LastFmTreeView( this );
        view->setFrameShape( QFrame::NoFrame );
        view->setDragEnabled ( true );
        view->setSortingEnabled( false );
        view->setDragDropMode ( QAbstractItemView::DragOnly );
        setView( view );
        setModel( new LastFmTreeModel( m_userName, this ) );

        //m_bottomPanel->setMaximumHeight( 300 );
        m_bottomPanel->hide();

        m_topPanel->setMaximumHeight( 300 );
        KHBox * outerProfilebox = new KHBox( m_topPanel );
        outerProfilebox->setSpacing(1);
        outerProfilebox->setMargin(0);

        m_avatarLabel = new QLabel(outerProfilebox);
        if( !m_avatar )
        {
            int m = dynamic_cast<LastFmTreeModel*>( model() )->avatarSize();
            m_avatarLabel->setPixmap( KIcon( "filename-artist-amarok" ).pixmap(m, m) );
            m_avatarLabel->setFixedSize( m, m );
        }
        else
        {
            m_avatarLabel->setPixmap( m_avatar );
            m_avatarLabel->setFixedSize( m_avatar.width(), m_avatar.height() );
            m_avatarLabel->setMargin( 5 );
        }

        debug() << m_avatarLabel->margin();
        KVBox * innerProfilebox = new KVBox( outerProfilebox );
        innerProfilebox->setSpacing(0);
        innerProfilebox->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
        m_userinfo = new QLabel(innerProfilebox);
        m_userinfo->setText( m_userName );
        m_profile = new QLabel(innerProfilebox);
        m_profile->setText(QString());
        updateProfileInfo();


        QGroupBox *customStation = new QGroupBox( i18n( "Create a Custom Last.fm Station" ), m_topPanel );
        m_customStationCombo = new QComboBox;
        QStringList choices;
        choices << i18n( "Artist" ) << i18n( "Tag" ) << i18n( "User" );
        m_customStationCombo->insertItems(0, choices);
        m_customStationEdit = new KLineEdit;
        m_customStationEdit->setClearButtonShown( true );
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

    if( lastfmTrack )
    {
        lastfmTrack->love();
        The::statusBar()->shortMessage( i18nc( "As in, lastfm", "Loved Track: %1", track->prettyName() ) );
    }
    else
    {
        m_scrobbler->loveTrack( track );
    }

}

void LastFmService::love( Meta::TrackPtr track )
{
    DEBUG_BLOCK
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
