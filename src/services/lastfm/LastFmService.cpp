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

#define DEBUG_PREFIX "LastFmService"
#include "core/support/Debug.h"

#include "LastFmService.h"

#include "AvatarDownloader.h"
#include "EngineController.h"
#include "biases/LastFmBias.h"
#include "biases/WeeklyTopBias.h"
#include "LastFmServiceCollection.h"
#include "LastFmServiceConfig.h"
#include "LoveTrackAction.h"
#include "SimilarArtistsAction.h"
#include "LastFmTreeModel.h"
#include "LastFmTreeView.h"
#include "ScrobblerAdapter.h"
#include "GlobalCurrentTrackActions.h"
#include "core/support/Components.h"
#include "core/interfaces/Logger.h"
#include "meta/LastFmMeta.h"
#include "SynchronizationAdapter.h"
#include "statsyncing/Controller.h"
#include "widgets/SearchWidget.h"

#include <KLineEdit>
#include <KStandardDirs>

#include <QCryptographicHash>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextDocument>        //Qt::escape
#include <QTimer>

#include <XmlQuery.h>

AMAROK_EXPORT_SERVICE_PLUGIN( lastfm, LastFmServiceFactory )

QString md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}

LastFmServiceFactory::LastFmServiceFactory( QObject *parent, const QVariantList &args )
    : ServiceFactory( parent, args )
{
    KPluginInfo pluginInfo(  "amarok_service_lastfm.desktop", "services" );
    pluginInfo.setConfig( config() );
    m_info = pluginInfo;
}

void
LastFmServiceFactory::init()
{
    ServiceBase *service = new LastFmService( this, "Last.fm" );
    m_initialized = true;
    emit newService( service );
}

QString
LastFmServiceFactory::name()
{
    return "Last.fm";
}

KConfigGroup
LastFmServiceFactory::config()
{
    return Amarok::config( LastFmServiceConfig::configSectionName() );
}

bool
LastFmServiceFactory::possiblyContainsTrack( const KUrl &url ) const
{
    return url.protocol() == "lastfm";
}


LastFmService::LastFmService( LastFmServiceFactory *parent, const QString &name )
    : ServiceBase( name, parent, false )
    , m_collection( 0 )
    , m_polished( false )
    , m_avatarLabel( 0 )
    , m_profile( 0 )
    , m_userinfo( 0 )
    , m_subscriber( false )
    , m_authenticateReply( 0 )
    , m_config( LastFmServiceConfig::instance() )
{
    DEBUG_BLOCK
    setShortDescription( i18n( "Last.fm: The social music revolution" ) );
    setIcon( KIcon( "view-services-lastfm-amarok" ) );
    setLongDescription( i18n( "Last.fm is a popular online service that provides personal radio stations and music recommendations. A personal listening station is tailored based on your listening habits and provides you with recommendations for new music. It is also possible to play stations with music that is similar to a particular artist as well as listen to streams from people you have added as friends or that Last.fm considers your musical \"neighbors\"" ) );
    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_lastfm.png" ) );

    //We have no use for searching currently..
    m_searchWidget->setVisible( false );

    // set the global static Lastfm::Ws stuff
    lastfm::ws::ApiKey = Amarok::lastfmApiKey();
    lastfm::ws::SharedSecret = Amarok::lastfmApiSharedSecret();

    // set the nam TWICE. Yes. It prevents liblastfm from deleting it, see their code
    lastfm::setNetworkAccessManager( The::networkAccessManager() );
    lastfm::setNetworkAccessManager( The::networkAccessManager() );

    // enable custom bias
    m_biasFactories << new Dynamic::LastFmBiasFactory();
    Dynamic::BiasFactory::instance()->registerNewBiasFactory( m_biasFactories.last() );
    m_biasFactories << new Dynamic::WeeklyTopBiasFactory();
    Dynamic::BiasFactory::instance()->registerNewBiasFactory( m_biasFactories.last() );

    // add the "play similar artists" action to all artist
    The::globalCollectionActions()->addArtistAction( new SimilarArtistsAction( this ) );
    The::globalCollectionActions()->addTrackAction( new LoveTrackAction( this ) );

    QAction *loveAction = new QAction( KIcon( "love-amarok" ), i18n( "Last.fm: Love" ), this );
    connect( loveAction, SIGNAL(triggered()), this, SLOT(love()) );
    loveAction->setShortcut( i18n( "Ctrl+L" ) );
    The::globalCurrentTrackActions()->addAction( loveAction );

    connect( m_config.data(), SIGNAL(updated()), this, SLOT(slotReconfigure()) );
    QTimer::singleShot(0, this, SLOT(slotReconfigure())); // call reconfigure but only after constructor is finished (because it might call virtual methods)
}

LastFmService::~LastFmService()
{
    DEBUG_BLOCK
    using namespace Dynamic;
    QMutableListIterator<AbstractBiasFactory *> it( m_biasFactories );
    while( it.hasNext() )
    {
        AbstractBiasFactory *factory = it.next();
        it.remove();

        BiasFactory::instance()->removeBiasFactory( factory );
        delete factory;
    }

    if( m_collection )
    {
        CollectionManager::instance()->removeTrackProvider( m_collection );
        m_collection->deleteLater();
        m_collection = 0;
    }

    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    if( m_scrobbler && controller )
        controller->unregisterScrobblingService( StatSyncing::ScrobblingServicePtr( m_scrobbler.data() ) );
    if( m_synchronizationAdapter && controller )
        controller->unregisterProvider( m_synchronizationAdapter );
}

void
LastFmService::slotReconfigure()
{
    lastfm::ws::Username = m_config->username();
    bool ready = !m_config->username().isEmpty(); // core features require just username

    /* create ServiceCollection only once the username is known (remember, getting
     * username from KWallet is async! */
    if( !m_collection && ready )
    {
        m_collection = new Collections::LastFmServiceCollection( m_config->username() );
        CollectionManager::instance()->addTrackProvider( m_collection );
    }

    // create Model once the username is known, it depends on it implicitly
    if( !model() && ready )
    {
        setModel( new LastFmTreeModel( this ) );
    }

    setServiceReady( ready ); // emits ready(), which needs to be done *after* creating collection

    // now authenticate w/ last.fm and get our session key if we don't have one
    if( !m_config->sessionKey().isEmpty() )
    {
        debug() << __PRETTY_FUNCTION__ << "using saved session key for last.fm";
        continueReconfiguring();
    }
    else if( !m_config->username().isEmpty() && !m_config->password().isEmpty() )
    {
        debug() << __PRETTY_FUNCTION__ << "got no saved session key, authenticating with last.fm";

        // discard any possible ongoing auth connections
        if( m_authenticateReply )
        {
            disconnect( m_authenticateReply, SIGNAL(finished()), this, SLOT(onAuthenticated()) );
            m_authenticateReply->abort();
            m_authenticateReply->deleteLater();
            m_authenticateReply = 0;
        }

        const QString authToken = md5( QString( "%1%2" ).arg( m_config->username(),
                md5( m_config->password().toUtf8() ) ).toUtf8() );
        QMap<QString, QString> query;
        query[ "method" ] = "auth.getMobileSession";
        query[ "username" ] = m_config->username();
        query[ "authToken" ] = authToken;
        m_authenticateReply = lastfm::ws::post( query );
        connect( m_authenticateReply, SIGNAL(finished()), this, SLOT(onAuthenticated()) ); // calls continueReconfiguring()
    }
    else
    {
        debug() << __PRETTY_FUNCTION__ << "either last.fm username or password is empty";
        continueReconfiguring();
    }
}

void
LastFmService::continueReconfiguring()
{
    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    Q_ASSERT( controller );

    lastfm::ws::SessionKey = m_config->sessionKey();
    // we also check username, KWallet may deliver it really late, but we need it
    bool authenticated = serviceReady() && !m_config->sessionKey().isEmpty();

    if( m_scrobbler && (!authenticated || !m_config->scrobble()) )
    {
        debug() << __PRETTY_FUNCTION__ << "unregistering and destorying ScrobblerAdapter";
        controller->unregisterScrobblingService( StatSyncing::ScrobblingServicePtr( m_scrobbler.data() ) );
        m_scrobbler = 0;
    }
    else if( !m_scrobbler && authenticated && m_config->scrobble() )
    {
        debug() << __PRETTY_FUNCTION__ << "creating and registering ScrobblerAdapter";
        m_scrobbler = new ScrobblerAdapter( "Amarok", m_config );
        controller->registerScrobblingService( StatSyncing::ScrobblingServicePtr( m_scrobbler.data() ) );
    }

    if( m_synchronizationAdapter && !authenticated )
    {
        debug() << __PRETTY_FUNCTION__ << "unregistering and destorying SynchronizationAdapter";
        controller->unregisterProvider( m_synchronizationAdapter );
        m_synchronizationAdapter = 0;
    }
    else if( !m_synchronizationAdapter && authenticated )
    {
        debug() << __PRETTY_FUNCTION__ << "creating and registering SynchronizationAdapter";
        m_synchronizationAdapter = new SynchronizationAdapter( m_config );
        controller->registerProvider( m_synchronizationAdapter );
    }

    // update possibly changed user info
    QNetworkReply *reply = lastfm::User::getInfo();
    connect( reply, SIGNAL(finished()), SLOT(onGetUserInfo()) );
}

void
LastFmService::onAuthenticated()
{
    if( !m_authenticateReply )
        warning() << __PRETTY_FUNCTION__ << "null reply!";
    else
        m_authenticateReply->deleteLater();

    /* temporarily disconnect form config updates to prevent calling
     * slotReconfigure() for the second time. */
    disconnect( m_config.data(), SIGNAL(updated()), this, SLOT(slotReconfigure()) );

    switch( m_authenticateReply ? m_authenticateReply->error() : QNetworkReply::UnknownNetworkError )
    {
        case QNetworkReply::NoError:
        {
            lastfm::XmlQuery lfm;
            if( !lfm.parse( m_authenticateReply->readAll() ) || lfm.children( "error" ).size() > 0 )
            {
                debug() << "error from authenticating with last.fm service:" << lfm.text();
                m_config->setSessionKey( QString() );
                m_config->save();
                break;
            }
            m_config->setSessionKey( lfm[ "session" ][ "key" ].text() );
            m_config->save();

            break;
        }
        case QNetworkReply::AuthenticationRequiredError:
            Amarok::Components::logger()->longMessage( i18nc("Last.fm: errorMessage",
                    "Either the username was not recognized, or the password was incorrect." ) );
            break;

        default:
            Amarok::Components::logger()->longMessage( i18nc("Last.fm: errorMessage",
                    "There was a problem communicating with the Last.fm services. Please try again later." ) );
            break;
    }
    m_authenticateReply = 0;

    // connect back to config updates
    connect( m_config.data(), SIGNAL(updated()), this, SLOT(slotReconfigure()) );
    continueReconfiguring();
}

void
LastFmService::onGetUserInfo()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
    if( !reply )
        warning() << __PRETTY_FUNCTION__ << "null reply!";
    else
        reply->deleteLater();

    switch( reply ? reply->error() : QNetworkReply::UnknownNetworkError )
    {
        case QNetworkReply::NoError:
        {
            lastfm::XmlQuery lfm;
            if( lfm.parse( reply->readAll() ) ) {
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
                    downloader->downloadAvatar( m_config->username(),  url);
                    connect( downloader, SIGNAL(avatarDownloaded(QString,QPixmap)),
                                         SLOT(onAvatarDownloaded(QString,QPixmap)) );
                }
                updateProfileInfo();
            }
            else
                debug() << "Got exception in parsing from last.fm:" << lfm.parseError().message();
            break;
        }
        case QNetworkReply::AuthenticationRequiredError:
            debug() << "Last.fm: errorMessage: Sorry, we don't recognise that username, or you typed the password incorrectly.";
            break;
        default:
            debug() << "Last.fm: errorMessage: There was a problem communicating with the Last.fm services. Please try again later.";
            break;
    }
}

void
LastFmService::onAvatarDownloaded( const QString &username, QPixmap avatar )
{
    DEBUG_BLOCK
    sender()->deleteLater();
    if( username == m_config->username() && !avatar.isNull() )
    {
        LastFmTreeModel* lfm = dynamic_cast<LastFmTreeModel*>( model() );
        if( !lfm )
            return;

        int m = lfm->avatarSize();
        avatar = avatar.scaled( m, m, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        lfm->prepareAvatar( avatar, m );
        m_avatar = avatar;

        if( m_avatarLabel )
            m_avatarLabel->setPixmap( m_avatar );
    }
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
            hint = i18n( "Enter a Last.fm user name" );
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
        m_userinfo->setText( i18n( "Username: %1", Qt::escape( m_config->username() ) ) );
    }

    if( m_profile && !m_playcount.isEmpty() )
    {
        m_profile->setText( i18np( "Play Count: %1 play", "Play Count: %1 plays", m_playcount.toInt() ) );
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

        //m_bottomPanel->setMaximumHeight( 300 );
        m_bottomPanel->hide();

        m_topPanel->setMaximumHeight( 300 );
        KHBox * outerProfilebox = new KHBox( m_topPanel );
        outerProfilebox->setSpacing(1);
        outerProfilebox->setMargin(0);

        m_avatarLabel = new QLabel(outerProfilebox);
        if( !m_avatar )
        {
            int m = LastFmTreeModel::avatarSize();
            m_avatarLabel->setPixmap( KIcon( "filename-artist-amarok" ).pixmap(m, m) );
            m_avatarLabel->setFixedSize( m, m );
        }
        else
        {
            m_avatarLabel->setPixmap( m_avatar );
            m_avatarLabel->setFixedSize( m_avatar.width(), m_avatar.height() );
            m_avatarLabel->setMargin( 5 );
        }

        KVBox * innerProfilebox = new KVBox( outerProfilebox );
        innerProfilebox->setSpacing(0);
        innerProfilebox->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
        m_userinfo = new QLabel(innerProfilebox);
        m_userinfo->setText( m_config->username() );
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

        connect( m_customStationEdit, SIGNAL(returnPressed()), this, SLOT(playCustomStation()) );
        connect( m_customStationButton, SIGNAL(clicked()), this, SLOT(playCustomStation()) );
        connect( m_customStationCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateEditHint(int)));

        QList<int> levels;
        levels << CategoryId::Genre << CategoryId::Album;
        m_polished = true;
    }
}

void
LastFmService::love()
{
    love( The::engineController()->currentTrack() );
}

void
LastFmService::love( Meta::TrackPtr track )
{
    if( m_scrobbler )
        m_scrobbler->loveTrack( track );
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
    The::playlistController()->insertOptioned( track, Playlist::OnPlayMediaAction );
}

Collections::Collection * LastFmService::collection()
{
    return m_collection;
}
