/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "Mp3tunesService.h"

#include "Mp3tunesWorkers.h"
#include "Mp3tunesConfig.h"

#include "Amarok.h"
#include "Debug.h"
#include "StatusBar.h"

#include <KMessageBox>
#include <KPasswordDialog>
#include <threadweaver/ThreadWeaver.h>

#include <QDomDocument>


AMAROK_EXPORT_PLUGIN( Mp3tunesServiceFactory )

void Mp3tunesServiceFactory::init()
{
    Mp3tunesConfig config;
    
    ServiceBase* service = new Mp3tunesService( "MP3tunes.com", config.email(), config.password() );
    m_activeServices << service;
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



Mp3tunesService::Mp3tunesService(const QString & name, const QString &email, const QString &password )
 : ServiceBase( name )
 , m_email( email )
 , m_password( password )
 , m_partnerToken( "7359149936" )
 , m_apiOutputFormat( "xml")
 , m_authenticated( false )
 , m_sessionId ( QString() )
{
    DEBUG_BLOCK
    setShortDescription( i18n( "The MP3tunes Locker service. Access your stored music!" ) );
    setIcon( KIcon( "view-services-mp3tunes-amarok" ) );
    debug() << "Making new Locker Object";
    m_locker = new Mp3tunesLocker( "7359149936" );
}


Mp3tunesService::~Mp3tunesService()
{
    delete m_locker;
}


void Mp3tunesService::polish()
{
    m_bottomPanel->hide();
    
    if ( !m_authenticated )
        authenticate( m_email, m_password );
}

void Mp3tunesService::authenticate( const QString & uname, const QString & passwd )
{
    DEBUG_BLOCK
    QString username, password;
   
    if ( uname.isEmpty() || passwd.isEmpty() ) {
        KPasswordDialog dlg( 0 , KPasswordDialog::ShowUsernameLine );  //FIXME 0x02 = KPasswordDialog::showUsername according to api, but that does not work
        dlg.setPrompt( i18n( "Enter your MP3tunes login and password" ) );
        if( !dlg.exec() )
            return; //the user canceled

        username = dlg.username();
        password = dlg.password();
    }
    else {
        username = uname;
        password = passwd;
    }

    Mp3tunesLoginWorker * loginWorker = new Mp3tunesLoginWorker( m_locker, username, password);
    debug() << "Connecting finishedLogin -> authentication complete.";
    connect( loginWorker, SIGNAL( finishedLogin( QString sessionId ) ), this, SLOT( authenticationComplete( QString sessionId ) ) );
    debug() << "Connection complete. Enqueueing..";
    ThreadWeaver::Weaver::instance()->enqueue( loginWorker );
    debug() << "LoginWorker queue";
    //The::statusBar()->newProgressOperation( m_xmlDownloadJob ).setDescription( i18n( "Authenticating" ) );
}


void Mp3tunesService::authenticationComplete( QString sessionId )
{
    DEBUG_BLOCK
    debug() << "Authentication reply: " << sessionId;
    if ( sessionId.isEmpty() )
    {
        KMessageBox::error( this, "errorMessage", i18n( "Authentication Error!" ) );
    } else {

    m_sessionId = sessionId;
    m_authenticated = true;

    m_collection = new Mp3tunesServiceCollection( m_sessionId );
    QList<int> levels;
    levels << CategoryId::Artist << CategoryId::Album;
    setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );

    }

}


#include "Mp3tunesService.moc"

