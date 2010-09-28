/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *           (c) 2010 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/


#include "AmpacheAccountLogin.h"

#include "sha256/sha256.h"

#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"



#include <QDomDocument>

#include <KPasswordDialog>
#include <KMessageBox>
#include <KMD5>

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


AmpacheAccountLogin::AmpacheAccountLogin( const QString& url, const QString& username, const QString& password, QWidget* parent )
    : QObject(parent)
    , m_authenticated( false )
    , m_server ( QString() )
    , m_sessionId ( QString() )
{
    //we are using http queries later on, we require http:// prefixed
    if( !url.contains( "://" ) )
    {
        m_server = QLatin1String("http://") + url;
    }
    else
        m_server = url;
    
    // We need to check the version of Ampache we are attempting to authenticate against, as this changes how we deal with it
    
    QString versionString = "<server>/server/xml.server.php?action=ping&user=<user>";
    
    versionString.replace(QString("<server>"), m_server);
    versionString.replace(QString("<user>"), username);
    
    debug() << "Verifying Ampache Version Using: " << versionString;
    
    m_username = username;
    m_password = password;
    
    m_xmlVersionUrl = KUrl( versionString );
    The::networkAccessManager()->getData( m_xmlVersionUrl, this,
                                          SLOT(authenticate(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}


AmpacheAccountLogin::~AmpacheAccountLogin()
{

}

void
AmpacheAccountLogin::reauthenticate()
{
    DEBUG_BLOCK
    
    debug() << " I am trying to re-authenticate";
    
    // We need to check the version of Ampache we are attempting to authenticate against, as this changes how we deal with it
    QString versionString = "<server>/server/xml.server.php?action=ping";
    
    versionString.replace(QString("<server>"), m_server);
    
    debug() << "Verifying Ampache Version Using: " << versionString;
    
    m_xmlVersionUrl = KUrl( versionString );
    The::networkAccessManager()->getData( m_xmlVersionUrl, this,
                                          SLOT(authenticate(KUrl,QByteArray,QNetworkReply::NetworkError)) );
}

void
AmpacheAccountLogin::authenticate( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( m_xmlVersionUrl != url )
        return;
    
    m_xmlVersionUrl.clear();
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "authenticate Error:" << e.description;
        return;
    }
    
    DEBUG_BLOCK
    versionVerify( data );
    
    //lets keep this around for now if we want to allow people to add a service that prompts for stuff
    if ( m_server.isEmpty() || m_password.isEmpty() )
    {
        KPasswordDialog dlg( 0 , KPasswordDialog::ShowUsernameLine );
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
        debug() << "Version Greater than 350000 Generating new SHA256 Auth" << authenticationString << passPhrase;
    }
    else
    {
        debug() << "Version Older than 35001 Generated MD5 Auth " << m_version;
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
    
    // TODO: Amarok::Components::logger()->newProgressOperation( m_xmlDownloadJob, i18n( "Authenticating with Ampache" ) );
    m_xmlDownloadUrl = KUrl( authenticationString );
    The::networkAccessManager()->getData( m_xmlDownloadUrl, this,
                                          SLOT(authenticationComplete(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void AmpacheAccountLogin::authenticationComplete( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( m_xmlDownloadUrl != url )
        return;
    
    m_xmlDownloadUrl.clear();
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Authentication Error:" << e.description;
        return;
    }
    
    QByteArray xmlReply( data );
    debug() << "Authentication reply: " << xmlReply;
    
    //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( xmlReply );
    QDomElement root = doc.firstChildElement("root");
    
    //is this an error?
    
    QDomElement domError = root.firstChildElement("error");
    
    if ( !domError.isNull() )
    {
        KMessageBox::error( qobject_cast<QWidget*>(parent()), domError.text(), i18n( "Authentication Error" ) );
    }
    else
    {
        //find status code:
        QDomElement element = root.firstChildElement("auth");
        m_sessionId = element.text();
        m_authenticated = true;
        emit loginSuccessful();
    }
    emit finished();
}

void AmpacheAccountLogin::versionVerify( QByteArray data)
{
    DEBUG_BLOCK
    QByteArray xmlReply = data;
    debug() << "Version Verify reply: " << xmlReply;
    
    //so lets figure out what we got here:
    QDomDocument doc( "version" );
    doc.setContent( xmlReply );
    QDomElement root = doc.firstChildElement("root");
    //is this an error?
    QDomElement error = root.firstChildElement("error");
    
    // It's OK if we get a null response from the version, that just means we're dealing with an older version
    if( !error.isNull() )
    {
        // Default the Version down if it didn't work
        m_version = 100000;
        debug() << "versionVerify Error: " << error.text();
    }
    else
    {
        //find status code:
        QDomElement element = root.firstChildElement("version");
        
        m_version = element.text().toInt();
        
        debug() << "versionVerify Returned: " << m_version;
    }
}

#include "AmpacheAccountLogin.moc"
