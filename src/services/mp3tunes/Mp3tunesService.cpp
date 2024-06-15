/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#define DEBUG_PREFIX "Mp3tunesService"

#include "Mp3tunesService.h"

#include "browsers/SingleCollectionTreeItemModel.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/logger/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "Mp3tunesConfig.h"

#include <QMenuBar>
#include <QRegularExpression>

#include <KMessageBox>
#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/Queue>


Mp3tunesServiceFactory::Mp3tunesServiceFactory()
    : ServiceFactory()
{}

void Mp3tunesServiceFactory::init()
{
    DEBUG_BLOCK
    ServiceBase *service = createService();
    if( service )
    {
        m_initialized = true;
        emit newService( service );
    }
}

ServiceBase* Mp3tunesServiceFactory::createService()
{
    Mp3tunesConfig config;
    //The user activated the service, but didn't fill the email/password? Don't start it.
    // if( config.email().isEmpty() || config.password().isEmpty() )
        // return 0;
    ServiceBase* service = new Mp3tunesService( this, "MP3tunes.com", config.partnerToken(), config.email(), config.password(),  config.harmonyEnabled() );
    return service;
}

QString Mp3tunesServiceFactory::name()
{
    return "MP3tunes.com";
}

KConfigGroup Mp3tunesServiceFactory::config()
{
    return Amarok::config( "Service_Mp3tunes" );
}


bool
Mp3tunesServiceFactory::possiblyContainsTrack(const QUrl &url) const
{
    QRegularExpression rx( "http://content.mp3tunes.com/storage/locker(?:get|play)/(.*)\\?(?:sid|partner_token)=.*" ) ;
    int matches = rx.indexIn( url.url() );
    if( matches == -1 ) {
        return false; // not a mp3tunes url
    }
    QStringList list = rx.capturedTexts();
    QString filekey = list.value( 1 ); // Because list[0] is the url itself.
    if ( filekey.isEmpty() ) {
        return false;
    }
    return true; // for now: if it's a mp3tunes url.. it's likely the track is in the locker
}


Mp3tunesService::Mp3tunesService( Mp3tunesServiceFactory* parent, const QString & name, const QString &token, const QString &email, const QString &password, bool harmonyEnabled )
 : ServiceBase( name, parent )
 , m_email( email )
 , m_password( password )
 , m_harmonyEnabled( harmonyEnabled )
 , m_partnerToken( token )
 , m_authenticated( false )
 , m_authenticationFailed( false )
 , m_sessionId ( QString() )
 , m_collection( 0 )
 , m_loginWorker( 0 )
 , m_harmony( 0 )
{
    DEBUG_BLOCK
    setShortDescription( i18n( "The MP3tunes Locker: Your Music Everywhere!" ) );
    setIcon( QIcon::fromTheme( "view-services-mp3tunes-amarok" ) );
    debug() << "Making new Locker Object";
    m_locker = new Mp3tunesLocker( "4895500420" );

    debug() << "MP3tunes running automated authenticate.  email: " << email << "  pass: " << password;
    authenticate( email, password );

    if( m_harmonyEnabled ) {
        enableHarmony();
    }

    polish();
}


Mp3tunesService::~Mp3tunesService()
{

    delete m_locker;
//    delete m_daemon;
    if( m_collection ) {
        CollectionManager::instance()->removeTrackProvider( m_collection );
        delete m_collection;
    }
}


void Mp3tunesService::polish()
{
    initTopPanel();
    initBottomPanel();

    if ( !m_authenticated && !m_authenticationFailed  )
        authenticate( m_email, m_password );
}

void Mp3tunesService::initTopPanel()
{
    m_menubar->clear();
    //Disable this menu bar until liblastfm is improved, and this feature can
    //be implemented correctly.
    /*QMenu * actionsMenu = m_menubar->addMenu( i18n( "AutoSync" ) );
    if( m_harmonyEnabled )
    {
        QAction * action = new QAction( i18n( "Disable AutoSync" ), m_menubar );
        connect( action, SIGNAL(triggered(bool)), SLOT(disableHarmony()) );
        actionsMenu->addAction( action );
    } else {
        QAction * action = new QAction( i18n( "Enable AutoSync" ), m_menubar );
        connect( action, SIGNAL(triggered(bool)), SLOT(enableHarmony()) );
        actionsMenu->addAction( action );
    }

    m_menubar->show();*/
}

void Mp3tunesService::initBottomPanel()
{
    m_bottomPanel->hide();
}

void Mp3tunesService::enableHarmony()
 {
    DEBUG_BLOCK

    if( !m_harmony )
    {
        debug() << "Making new Daemon";
        Mp3tunesConfig config;
        debug () << "Using identifier: " << config.identifier();

        if( config.pin().isEmpty() )
            m_harmony = new Mp3tunesHarmonyHandler( config.identifier() ); //first time harmony login
        else
            m_harmony = new Mp3tunesHarmonyHandler( config.identifier(), //they're not harmony virgins
                                                config.email(),
                                                config.pin() );
//        qRegisterMetaType<Mp3tunesHarmonyDownload>("Mp3tunesHarmonyDownload");

        connect( m_harmony, &Mp3tunesHarmonyHandler::disconnected,
                 this, &Mp3tunesService::harmonyDisconnected );
        connect( m_harmony, &Mp3tunesHarmonyHandler::waitingForEmail,
                 this, &Mp3tunesService::harmonyWaitingForEmail );
        connect( m_harmony, &Mp3tunesHarmonyHandler::waitingForPin,
                 this, &Mp3tunesService::harmonyWaitingForPin );
        connect( m_harmony, &Mp3tunesHarmonyHandler::connected,
                 this, &Mp3tunesService::harmonyConnected );
        connect( m_harmony, &Mp3tunesHarmonyHandler::signalError,
                 this, &Mp3tunesService::harmonyError );
        connect( m_harmony, &Mp3tunesHarmonyHandler::downloadReady,
                 this, &Mp3tunesService::harmonyDownloadReady );
        connect( m_harmony, &Mp3tunesHarmonyHandler::downloadPending,
                 this, &Mp3tunesService::harmonyDownloadPending );

        debug() << "starting harmony";
        m_harmony->startDaemon();
        if( m_harmony->daemonRunning() )
        {
            debug() << "harmony started.. making connection";
            m_harmony->makeConnection();
        }
        if( m_harmony->daemonConnected() )
            debug() << "harmony connected";
        else
            debug() << "harmony failed to connected";

        //Close your eyes. Cross your legs. Touch middle fingers to thumbs. Extend your arms.
        //OOOooommmmm
    }

    debug() << "Daemon running";
    m_harmonyEnabled = true;
    Amarok::Logger::shortMessage( i18n( "MP3tunes AutoSync Enabled"  ) );
    polish();
 }

 void Mp3tunesService::disableHarmony()
 {
    DEBUG_BLOCK
    if( !m_harmony )
        return;

    debug()  << "stopping daemon";
    m_harmony->stopDaemon();
    m_harmony = 0;
    m_harmonyEnabled = false;
    polish();

    Amarok::Logger::shortMessage( i18n( "MP3tunes AutoSync Disabled"  ) );
 }

void Mp3tunesService::authenticate( const QString & uname, const QString & passwd )
{
    DEBUG_BLOCK
    if( m_loginWorker )
        return;

    if ( uname.isEmpty() || passwd.isEmpty() )
       return;

    m_loginWorker = new Mp3tunesLoginWorker( m_locker, uname, passwd);
    //debug() << "Connecting finishedLogin -> authentication complete.";

    connect( m_loginWorker, &Mp3tunesLoginWorker::finishedLogin,
             this, &Mp3tunesService::authenticationComplete );
    //debug() << "Connection complete. Enqueueing..";
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(m_loginWorker) );
    //debug() << "LoginWorker queue";
    Amarok::Logger::shortMessage( i18n( "Authenticating"  ) );

}


void Mp3tunesService::authenticationComplete( const QString & sessionId )
{
    DEBUG_BLOCK
    m_loginWorker = 0;
    debug() << "Authentication reply: " << sessionId;
    if ( sessionId.isEmpty() )
    {
        QString error = i18n("MP3tunes failed to Authenticate.");
        if ( !m_locker->errorMessage().isEmpty() )
        {
            error = m_locker->errorMessage(); // Not sure how to i18n this
        }
        Amarok::Logger::longMessage( error );

        setServiceReady( false );
        m_authenticationFailed = true;
    }
    else
    {
        m_sessionId = sessionId;
        m_authenticated = true;

        m_collection = new Collections::Mp3tunesServiceCollection( this, m_sessionId, m_locker );
        CollectionManager::instance()->addTrackProvider( m_collection );
        QList<CategoryId::CatMenuId> levels;
        levels << CategoryId::Artist << CategoryId::Album;
        setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );

        setServiceReady( true );
    }
    polish();
}

void Mp3tunesService::harmonyDisconnected()
{
    DEBUG_BLOCK
    debug() << "Harmony Disconnected!";
    Amarok::Logger::shortMessage( i18n( "MP3tunes Harmony: Disconnected"  ) );
}

void Mp3tunesService::harmonyWaitingForEmail( const QString &pin )
{
    DEBUG_BLOCK
    debug() << "Waiting for user to input PIN: " << pin;
    Amarok::Logger::shortMessage( i18n( "MP3tunes Harmony: Waiting for PIN Input"  ) );
    KMessageBox::information( this,
                              "Please go to <a href=\"http://www.mp3tunes.com/pin\">mp3tunes.com/pin</a> and enter the following pin.\n\tPIN: " + pin,
                              "MP3tunes Harmony",
                              QString(),
                              KMessageBox::AllowLink );
}

void Mp3tunesService::harmonyWaitingForPin()
{
    DEBUG_BLOCK
    QString pin = m_harmony->pin();
    debug() << "Waiting for user to input PIN: " << pin;
    Amarok::Logger::shortMessage( i18n( "MP3tunes Harmony: Waiting for PIN Input"  ) );
    KMessageBox::information( this,
                              "Please go to <a href=\"http://www.mp3tunes.com/pin\">mp3tunes.com/pin</a> and enter the following pin.\n\tPIN: " + pin,
                              "MP3tunes Harmony",
                              QString(),
                              KMessageBox::AllowLink );
}

void Mp3tunesService::harmonyConnected()
{
    DEBUG_BLOCK
    debug() << "Harmony Connected!";
    Amarok::Logger::shortMessage( i18n( "MP3tunes Harmony: Successfully Connected"  ) );
    /* at this point since the user has input the pin, we will save the info
       for later authentication*/
    Mp3tunesConfig config;
    debug() << "Setting Config   email: " << m_harmony->email() << "   pin: " << m_harmony->pin();
    config.setHarmonyEmail( m_harmony->email() );
    config.setPin( m_harmony->pin() );
    config.save();

}

void Mp3tunesService::harmonyError( const QString &error )
{
    DEBUG_BLOCK
    debug() << "Harmony Error: " << error;
    Amarok::Logger::longMessage( i18n( "MP3tunes Harmony Error\n%1", error ) );
}

void Mp3tunesService::harmonyDownloadReady( const QVariantMap &download )
{
    DEBUG_BLOCK
    debug() << "Got message about ready: " << download["trackTitle"] << " by " << download["artistName"] << " on " << download["albumTitle"];
    foreach( Collections::Collection *coll, CollectionManager::instance()->collections().keys() )
    {
        if( coll && coll->isWritable() && m_collection )
        {
            debug() << "got collection" << coll->prettyName();
            if ( coll->collectionId() == "localCollection")
            { //TODO Allow user to choose which collection to sync down to.
                debug() << "got local collection";
                Collections::CollectionLocation *dest = coll->location();
                Collections::CollectionLocation *source = m_collection->location();
                if( !m_collection->possiblyContainsTrack( download["url"].toUrl() ) )
                    return; //TODO some sort of error handling
                Meta::TrackPtr track( m_collection->trackForUrl( download["url"].toUrl() ) );
                source->prepareCopy( track, dest );
                break;
            }

        }
    }

}

void Mp3tunesService::harmonyDownloadPending( const QVariantMap &download )
{
    DEBUG_BLOCK
    debug() << "Got message about ready: " << download["trackTitle"] << " by " << download["artistName"] << " on " << download["albumTitle"];
}
