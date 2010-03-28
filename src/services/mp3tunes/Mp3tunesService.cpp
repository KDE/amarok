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

#include "Mp3tunesService.h"

#include "browsers/SingleCollectionTreeItemModel.h"
#include "CollectionManager.h"
#include "core/support/Debug.h"
#include "Mp3tunesConfig.h"
#include "statusbar/StatusBar.h"

#include <KMenuBar>
#include <KMessageBox>
#include <threadweaver/ThreadWeaver.h>

#include <QRegExp>

AMAROK_EXPORT_PLUGIN( Mp3tunesServiceFactory )

void Mp3tunesServiceFactory::init()
{
    Mp3tunesConfig config;

    //The user activated the service, but didn't fill the email/password? Don't start it.
    if ( config.email().isEmpty() || config.password().isEmpty() )
        return;
    
    ServiceBase* service = new Mp3tunesService( this, "MP3tunes.com", config.partnerToken(), config.email(), config.password(),  config.harmonyEnabled() );
    m_activeServices << service;
    m_initialized = true;
    connect( service, SIGNAL( ready() ), this, SLOT( slotServiceReady() ) );
    emit newService( service );
    
}


QString Mp3tunesServiceFactory::name()
{
    return "MP3tunes.com";
}


KPluginInfo Mp3tunesServiceFactory::info()
{
    KPluginInfo pluginInfo(  "amarok_service_mp3tunes.desktop", "services" );
    pluginInfo.setConfig( config() );
    return pluginInfo;
}


KConfigGroup Mp3tunesServiceFactory::config()
{
    return Amarok::config( "Service_Mp3tunes" );
}


bool
Mp3tunesServiceFactory::possiblyContainsTrack(const KUrl & url) const
{
    QRegExp rx( "http://content.mp3tunes.com/storage/locker(?:get|play)/(.*)\\?(?:sid|partner_token)=.*" ) ;
    int matches = rx.indexIn( url.url() );
    if( matches == -1 ) {
        debug() << "not a track no match";
        return false; // not a mp3tunes url
    }
    QStringList list = rx.capturedTexts();
    QString filekey = list[1]; // Because list[0] is the url itself.
    if ( filekey.isEmpty() ) {
        debug() << "not a track bad url";
        return false;
    }
    debug() << "is a track!";
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
    setIcon( KIcon( "view-services-mp3tunes-amarok" ) );
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
        CollectionManager::instance()->removeUnmanagedCollection( m_collection );
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
        connect( action, SIGNAL( triggered( bool) ), SLOT( disableHarmony() ) );
        actionsMenu->addAction( action );
    } else {
        QAction * action = new QAction( i18n( "Enable AutoSync" ), m_menubar );
        connect( action, SIGNAL( triggered( bool) ), SLOT( enableHarmony() ) );
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

        connect( m_harmony, SIGNAL( disconnected() ),
                this, SLOT( harmonyDisconnected() ));
        connect( m_harmony, SIGNAL( waitingForEmail( QString ) ),
                this, SLOT( harmonyWaitingForEmail( QString ) ) );
        connect( m_harmony, SIGNAL( waitingForPin() ),
                 this, SLOT( harmonyWaitingForPin() ) );
        connect( m_harmony, SIGNAL( connected() ),
                this, SLOT( harmonyConnected() ) );
        connect( m_harmony, SIGNAL( signalError( QString ) ),
                this, SLOT( harmonyError( QString ) ) );
        connect( m_harmony, SIGNAL( downloadReady( QVariantMap ) ),
                this, SLOT( harmonyDownloadReady( QVariantMap ) ) );
        connect( m_harmony, SIGNAL( downloadPending( QVariantMap ) ),
                this, SLOT( harmonyDownloadPending( QVariantMap ) ) );

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
    The::statusBar()->shortMessage( i18n( "MP3tunes AutoSync Enabled"  ) );
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

    The::statusBar()->shortMessage( i18n( "MP3tunes AutoSync Disabled"  ) );
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

    connect( m_loginWorker, SIGNAL( finishedLogin( QString ) ), this,
             SLOT( authenticationComplete( QString ) ) );
    //debug() << "Connection complete. Enqueueing..";
    ThreadWeaver::Weaver::instance()->enqueue( m_loginWorker );
    //debug() << "LoginWorker queue";
    The::statusBar()->shortMessage( i18n( "Authenticating"  ) );

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
        The::statusBar()->longMessage( error );

        m_serviceready = false;
        m_authenticationFailed = true;

    }
    else
    {
        m_sessionId = sessionId;
        m_authenticated = true;

        m_collection = new Mp3tunesServiceCollection( this, m_sessionId, m_locker );
        CollectionManager::instance()->addUnmanagedCollection( m_collection,
                                    CollectionManager::CollectionDisabled );
        QList<int> levels;
        levels << CategoryId::Artist << CategoryId::Album;
        setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );

        m_serviceready = true;
        emit( ready() );
    }
    polish();
}

void Mp3tunesService::harmonyDisconnected()
{
    DEBUG_BLOCK
    debug() << "Harmony Disconnected!";
    The::statusBar()->shortMessage( i18n( "MP3tunes Harmony: Disconnected"  ) );
}

void Mp3tunesService::harmonyWaitingForEmail( const QString &pin )
{
    DEBUG_BLOCK
    debug() << "Waiting for user to input PIN: " << pin;
    The::statusBar()->shortMessage( i18n( "MP3tunes Harmony: Waiting for PIN Input"  ) );
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
    The::statusBar()->shortMessage( i18n( "MP3tunes Harmony: Waiting for PIN Input"  ) );
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
    The::statusBar()->shortMessage( i18n( "MP3tunes Harmony: Successfully Connected"  ) );
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
    The::statusBar()->longMessage( i18n( "MP3tunes Harmony Error\n%1", error ) );
}

void Mp3tunesService::harmonyDownloadReady( const QVariantMap &download )
{
    DEBUG_BLOCK
    debug() << "Got message about ready: " << download["trackTitle"].toString() << " by " << download["artistName"].toString() << " on " << download["albumTitle"].toString();
    foreach( Collections::Collection *coll, CollectionManager::instance()->collections().keys() ) {
        if( coll && coll->isWritable() && m_collection )
        {
            debug() << "got collection" << coll->prettyName();
            if ( coll->prettyName() == "Local Collection")
            { //TODO Allow user to choose which collection to sync down to.
                debug() << "got local collection";
                CollectionLocation *dest = coll->location();
                CollectionLocation *source = m_collection->location();
                if( !m_collection->possiblyContainsTrack( download["url"].toString() ) )
                    return; //TODO some sort of error handling
                Meta::TrackPtr track( m_collection->trackForUrl( download["url"].toString() ) );
                source->prepareCopy( track, dest );
                break;
            }

        }
    }

}

void Mp3tunesService::harmonyDownloadPending( const QVariantMap &download )
{
    DEBUG_BLOCK
    debug() << "Got message about ready: " << download["trackTitle"].toString() << " by " << download["artistName"].toString() << " on " << download["albumTitle"].toString();
}

#include "Mp3tunesService.moc"

