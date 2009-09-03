/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "AmpacheService.h"

#include "Amarok.h"
#include "AmpacheConfig.h"
#include "collection/CollectionManager.h"
#include "Debug.h"
#include "sha256/sha256.h"
#include "statusbar/StatusBar.h"


#include <KMessageBox>
#include <kpassworddialog.h>
#include <KMD5>


#include <QDomDocument>

AMAROK_EXPORT_PLUGIN( AmpacheServiceFactory )

QString sha256( QString in )
{
    unsigned char digest[ SHA512_DIGEST_SIZE];
    unsigned char* toHash = (unsigned char*)in.toUtf8().data();
    
    sha256( toHash , qstrlen( ( char* )toHash ), digest );

    // this part copied from main() in sha256.cpp
    unsigned char output[2 * SHA512_DIGEST_SIZE + 1];
    int i;

    output[2 * SHA256_DIGEST_SIZE ] = '\0';

    for (i = 0; i < SHA256_DIGEST_SIZE ; i++) {
       sprintf((char *) output + 2*i, "%02x", digest[i]);
    }
    
    return QString::fromAscii( (const char*)output );
}

void AmpacheServiceFactory::init()
{
    //read config and create the needed number of services
    AmpacheConfig config;
    AmpacheServerList servers = config.servers();
    m_initialized = true;

    for( int i = 0; i < servers.size(); i++ )
    {
        AmpacheServerEntry server = servers.at( i );
        ServiceBase* service = new AmpacheService( this, "Ampache (" + server.name + ')', server.url, server. username, server.password );
        m_activeServices << service;
        debug() << "Emitting service!!!!!!";
        connect( service, SIGNAL( ready() ), this, SLOT( slotServiceReady() ) );
        emit newService( service );
    }
}

QString
AmpacheServiceFactory::name()
{
    return "Ampache";
}

KPluginInfo
AmpacheServiceFactory::info()
{
    KPluginInfo pluginInfo( "amarok_service_ampache.desktop", "services" );
    pluginInfo.setConfig( config() );
    return pluginInfo;
}

KConfigGroup
AmpacheServiceFactory::config()
{
    return Amarok::config( "Service_Ampache" );
}

bool
AmpacheServiceFactory::possiblyContainsTrack(const KUrl & url) const
{
    AmpacheConfig config;
    foreach( const AmpacheServerEntry &server, config.servers() )
    {
        if ( url.url().contains( server.url, Qt::CaseInsensitive ) )
            return true;
    }

    return false;
}


AmpacheService::AmpacheService( AmpacheServiceFactory* parent, const QString & name, const QString &url, const QString &username, const QString &password )
    : ServiceBase( name,  parent )
    , m_authenticated( false )
    , m_server ( QString() )
    , m_sessionId ( QString() )
    , m_collection( 0 )
{
    DEBUG_BLOCK


    setShortDescription( i18n( "Amarok frontend for your Ampache server." ) );
    setIcon( KIcon( "view-services-ampache-amarok" ) );
    setLongDescription( i18n( "Use Amarok as a seamless frontend to your Ampache server. This lets you browse and play all the Ampache contents from within Amarok." ) );
    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_ampache.png" ) );

    //we are using http queries later on, so we require
    KUrl kurl;
    if( url.contains( "//" ) )
    {
        kurl.setUrl( url, KUrl::TolerantMode );
        if( kurl.protocol() != "http" && kurl.protocol() != "https" )
        {
            kurl.setProtocol( "http" );
        }
    }
    else
    {
        kurl.setProtocol( "http" );
        kurl.setAuthority( url );
    }

    m_server = kurl.url();

    // We need to check the version of Ampache we are attempting to authenticate against, as this changes how we deal with it

    QString versionString = "<server>/server/xml.server.php?action=ping";

    versionString.replace(QString("<server>"), m_server);

    debug() << "Verifying Ampache Version Using: " << versionString;

    m_username = username;
    m_password = password;

    m_xmlVersionJob = KIO::storedGet( versionString, KIO::Reload, KIO::HideProgressInfo );
    connect( m_xmlVersionJob, SIGNAL(result(KJob *)), this, SLOT( authenticate(KJob *) ) );
}

AmpacheService::~AmpacheService()
{
    CollectionManager::instance()->removeUnmanagedCollection( m_collection );
    delete m_collection;
}

void
AmpacheService::polish()
{
    m_bottomPanel->hide();

    /*if ( !m_authenticated )
        authenticate( );*/
}

void
AmpacheService::reauthenticate()
{
    DEBUG_BLOCK

    debug() << " I am trying to re-authenticate"; 

    // We need to check the version of Ampache we are attempting to authenticate against, as this changes how we deal with it
    QString versionString = "<server>/server/xml.server.php?action=ping";

    versionString.replace(QString("<server>"), m_server);

    debug() << "Verifying Ampache Version Using: " << versionString;

    m_xmlVersionJob = KIO::storedGet( versionString, KIO::Reload, KIO::HideProgressInfo );
    connect( m_xmlVersionJob, SIGNAL(result(KJob *)), this, SLOT( authenticate(KJob *) ) );
}

void
AmpacheService::authenticate(KJob * job)
{
    DEBUG_BLOCK

    versionVerify(job); 

    //lets keep this around for now if we want to allow people to add a service that prompts for stuff
    if ( m_server.isEmpty() || m_password.isEmpty() )
    {
        KPasswordDialog dlg( 0 , KPasswordDialog::ShowUsernameLine );  //FIXME 0x02 = KPasswordDialog::showUsername according to the API, but that does not work
        dlg.setPrompt( i18n( "Enter the server name and a password" ) );
        if( !dlg.exec() )
            return; //the user canceled

        KUrl kurl( dlg.username() );
        if( kurl.protocol() != "http" && kurl.protocol() != "https" )
        {
            kurl.setProtocol( "http" );
        }
        m_server = kurl.url();
        m_password = dlg.password();
    }
    else
    {
        KUrl kurl( m_server );
        if( kurl.protocol() != "http" && kurl.protocol() != "https" )
        {
            kurl.setProtocol( "http" );
        }
        m_server = kurl.url();
    }

    QString timestamp = QString::number( QDateTime::currentDateTime().toTime_t() );

    QString rawHandshake;
    QString authenticationString;
    QString passPhrase;

    // We need to use different authentication strings depending on the version of ampache
    if ( m_version > 350000 )
    {
	debug() << "New Password Scheme " << m_version; 
        authenticationString = "<server>/server/xml.server.php?action=handshake<username>&auth=<passphrase>&timestamp=<timestamp>&version=350001";

        rawHandshake = timestamp + sha256( m_password );

        passPhrase = sha256( rawHandshake );
	debug() << "Version Greater then 35001 Generating new SHA256 Auth" << authenticationString << passPhrase;
    }
    else { 
        debug() << "Version Older then 35001 Generated MD5 Auth " << m_version;
        authenticationString = "<server>/server/xml.server.php?action=handshake<username>&auth=<passphrase>&timestamp=<timestamp>";
        rawHandshake = timestamp + m_password;
        KMD5 context( rawHandshake.toUtf8() );
        passPhrase = context.hexDigest().data();
    }

    authenticationString.replace(QString("<server>"), m_server);
    if ( !m_username.isEmpty() )
        authenticationString.replace(QString("<username>"), "&user=" + m_username);
    else
        authenticationString.remove(QString("<username>"));
    authenticationString.replace(QString("<passphrase>"), passPhrase);
    authenticationString.replace(QString("<timestamp>"), timestamp);

    debug() << "Authenticating with string: " << authenticationString << passPhrase;

    m_xmlDownloadJob = KIO::storedGet( authenticationString, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_xmlDownloadJob, SIGNAL(result(KJob *)), this, SLOT( authenticationComplete( KJob*) ) );
    The::statusBar()->newProgressOperation( m_xmlDownloadJob, i18n( "Authenticating with Ampache" ) );
}

void AmpacheService::authenticationComplete(KJob * job)
{
    if( !job->error() == 0 )
    {
        //TODO: error handling here
        return;
    }
    if( job != m_xmlDownloadJob )
        return; //not the right job, so let's ignore it

    QString xmlReply = ((KIO::StoredTransferJob* )job)->data();
    debug() << "Authentication reply: " << xmlReply;

    //so lets figure out what we got here:
    QDomDocument doc( "reply" );

    doc.setContent( m_xmlDownloadJob->data() );
    QDomElement root = doc.firstChildElement("root");

    //is this an error?

    QDomElement error = root.firstChildElement("error");

    if ( !error.isNull() )
    {
        KMessageBox::error ( this, error.text(), i18n( "Authentication Error" ) );
    }
    else
    {
        //find status code:
        QDomElement element = root.firstChildElement("auth");

        m_sessionId = element.text();

        m_authenticated = true;

        m_collection = new AmpacheServiceCollection( this, m_server, m_sessionId );
        connect( m_collection, SIGNAL( authenticationNeeded() ), SLOT( authenticate() ) );

        CollectionManager::instance()->addUnmanagedCollection( m_collection, CollectionManager::CollectionDisabled );
        QList<int> levels;
        levels << CategoryId::Artist << CategoryId::Album;
        setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );
        m_serviceready = true;
        emit( ready() );

        m_xmlDownloadJob->deleteLater();
    }

}

void AmpacheService::versionVerify(KJob * job)
{

    DEBUG_BLOCK

    if( !job->error() == 0 )
    {
	debug() << "Job Error" << job->error();
	// If an error has occurred, it's non-fatal unless they are using 3.5, as we default to 3.4 currently
        return;
    }
    QString xmlReply = ((KIO::StoredTransferJob* )job)->data();
    debug() << "Version Verify reply: " << xmlReply;

    //so lets figure out what we got here:
    QDomDocument doc( "version" );

    doc.setContent(m_xmlVersionJob->data() );
    QDomElement root = doc.firstChildElement("root");
    //is this an error?

    QDomElement error = root.firstChildElement("error");

    // It's OK if we get a null response from the version, that just means we're dealing with an older version
    if ( !error.isNull() )
    {
	// Default the Version down if it didn't work 
	m_version = 100000;
	debug() << "AmpacheService::versionVerify Error: " << error.text();
    }
    else
    {
        //find status code:
        QDomElement element = root.firstChildElement("version");

        m_version = element.text().toInt();

        debug() << "versionVerify Returned: " << m_version;

        job->deleteLater();
    }

}

#include "AmpacheService.moc"

