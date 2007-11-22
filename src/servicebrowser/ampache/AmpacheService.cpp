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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "AmpacheService.h"

#include "amarok.h"
#include "collection/CollectionManager.h"
#include "debug.h"
#include "statusbar.h"


#include <KMessageBox>
#include <kpassworddialog.h>
#include <KMD5>


AmpacheService::AmpacheService(const QString & name)
 : ServiceBase( name )
 , m_partnerToken( "7359149936" )
 , m_apiOutputFormat( "xml")
 , m_authenticated( false )
 , m_server ( QString() )
 , m_sessionId ( QString() )
 , m_collection( 0 )
{

    setShortDescription("Use Amarok as a seamless frontend to your Ampache server!");
    setIcon( KIcon( Amarok::icon( "download" ) ) );
    showInfo( false );

}


AmpacheService::~AmpacheService()
{
    CollectionManager::instance()->removeUnmanagedCollection( m_collection );
    delete m_collection;
}

void AmpacheService::polish()
{
    if ( !m_authenticated )
        authenticate();

}

void AmpacheService::authenticate( const QString & serv, const QString & passwd )
{

QString server, password;
   
    if ( server.isEmpty() || passwd.isEmpty() ) {
        KPasswordDialog dlg( 0 , KPasswordDialog::ShowUsernameLine );  //FIXME 0x02 = KPasswordDialog::showUsername according to api, but that does not work
        dlg.setPrompt( i18n( "Enter the server name and a password" ) );
        if( !dlg.exec() )
            return; //the user canceled

        m_server = dlg.username();
        password = dlg.password();

    } else {
        m_server = serv;
        password = passwd;
    }

    
    QString timestamp = QString::number( QDateTime::currentDateTime().toTime_t() );

    QString rawHandshake = timestamp + password;
    KMD5 context( rawHandshake.toUtf8() );
    
    
    QString passPhrase = context.hexDigest().data();

    QString authenticationString = "<server>/server/xml.server.php?action=handshake&auth=<passphrase>&timestamp=<timestamp>";

    authenticationString.replace(QString("<server>"), m_server);
    authenticationString.replace(QString("<passphrase>"), passPhrase);
    authenticationString.replace(QString("<timestamp>"), timestamp);


    debug() << "Authenticating with string: " << authenticationString;



    m_xmlDownloadJob = KIO::storedGet( authenticationString, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_xmlDownloadJob, SIGNAL(result(KJob *)), this, SLOT( authenticationComplete( KJob*) ) );
    Amarok::StatusBar::instance() ->newProgressOperation( m_xmlDownloadJob )
    .setDescription( i18n( "Authenticating" ) );

}

void AmpacheService::authenticationComplete(KJob * job)
{

    if ( !job->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }
    if ( job != m_xmlDownloadJob )
        return ; //not the right job, so let's ignore it


    QString xmlReply = ((KIO::StoredTransferJob* )job)->data();
    debug() << "Authentication reply: " << xmlReply;


    //so lets figure out what we got here:
    QDomDocument doc( "reply" );

    doc.setContent( m_xmlDownloadJob->data() );

   
    QDomElement root = doc.firstChildElement("root");

    
    //is this an error?

    QDomElement error = root.firstChildElement("error");

    if ( !error.isNull() ) {
        KMessageBox::error ( this, error.text(), "Authentication Error!" );
    } else {
    
        //find status code:
        QDomElement element = root.firstChildElement("auth");

        m_sessionId = element.text();

        m_authenticated = true;

        m_collection = new AmpacheServiceCollection( m_server, m_sessionId );
        CollectionManager::instance()->addUnmanagedCollection( m_collection );
        QList<int> levels;
        levels << CategoryId::Artist << CategoryId::Album;
        setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );

    }

    m_xmlDownloadJob->deleteLater();
}

#include "AmpacheService.moc"


