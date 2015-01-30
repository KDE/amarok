/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *           (c) 2010 Ian Monroe <ian@monroe.nu>                                        *
 *           (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"

#include <QDomDocument>
#include <QNetworkRequest>
#include <QtCrypto>

#include <KLocale>
#include <KPasswordDialog>
#include <KMessageBox>


AmpacheAccountLogin::AmpacheAccountLogin( const QString& url, const QString& username, const QString& password, QWidget* parent )
    : QObject(parent)
    , m_authenticated( false )
    , m_server( url )
    , m_username( username )
    , m_password( password )
    , m_sessionId( QString() )
    , m_lastRequest( 0 )
{
    reauthenticate();

}


AmpacheAccountLogin::~AmpacheAccountLogin()
{

}

void
AmpacheAccountLogin::reauthenticate()
{
    DEBUG_BLOCK

    // We need to check the version of Ampache we are attempting to authenticate against, as this changes how we deal with it
    KUrl url = getRequestUrl( "ping" );

    debug() << "Verifying Ampache Version Using: " << url.url();

    m_lastRequest = The::networkAccessManager()->getData( url, this,
                                                          SLOT(authenticate(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );

    if( !m_lastRequest )
        emit finished();
}

void
AmpacheAccountLogin::authenticate( const KUrl &requestUrl, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( !m_lastRequest )
        return;

    QCA::Initializer init;

    DEBUG_BLOCK
    Q_UNUSED( requestUrl );

    QDomDocument doc;
    doc.setContent( data );

    if( !generalVerify( doc, e ) )
        return;

    // so lets figure out what we got here:
    debug() << "Version reply: " << data;
    int version = getVersion( doc );

    KUrl url = getRequestUrl( "handshake" );
    QString timestamp = QString::number( QDateTime::currentDateTime().toTime_t() );
    QString passPhrase;

    // We need to use different authentication strings depending on the version of ampache
    if( version > 350000 )
    {



        debug() << "New Password Scheme " << version;
        url.addQueryItem( "version", "350001" );

        QCA::Hash sha256Hash( "sha256" );
        sha256Hash.update( m_password.toUtf8() );
        QString hashedPassword = QCA::arrayToHex( sha256Hash.final().toByteArray() );

        QString rawHandshake = timestamp + hashedPassword;
        sha256Hash.clear();
        sha256Hash.update( rawHandshake.toUtf8() );

        passPhrase = QCA::arrayToHex( sha256Hash.final().toByteArray() );

    }
    else
    {
        debug() << "Version Older than 35001 Generated MD5 Auth " << version;

        QString rawHandshake = timestamp + m_password;
        QCA::Hash md5Hash( "md5" );

        md5Hash.update( rawHandshake.toUtf8() );
        passPhrase = QCA::arrayToHex( md5Hash.final().toByteArray() );
    }

    url.addQueryItem( "timestamp", timestamp );
    url.addQueryItem( "auth", passPhrase );

    debug() << "Authenticating with string: " << url.url() << passPhrase;

    // TODO: Amarok::Components::logger()->newProgressOperation( m_xmlDownloadJob, i18n( "Authenticating with Ampache" ) );
    m_lastRequest = The::networkAccessManager()->getData( url, this,
                                                          SLOT(authenticationComplete(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );

    if( !m_lastRequest )
        emit finished();
}

void AmpacheAccountLogin::authenticationComplete( const KUrl &requestUrl, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( !m_lastRequest )
        return;

    DEBUG_BLOCK
    Q_UNUSED( requestUrl );

    QDomDocument doc;
    doc.setContent( data );

    if( !generalVerify( doc, e ) )
        return;

    // so lets figure out what we got here:
    debug() << "Authentication reply: " << data;
    QDomElement root = doc.firstChildElement("root");

    //find status code:
    QDomElement element = root.firstChildElement("auth");
    if( element.isNull() )
    {
        // Default the Version down if it didn't work
        debug() << "authenticationComplete failed";
        KMessageBox::error( qobject_cast<QWidget*>(parent()),
                            i18n( "Authentication failed." ),
                            i18n( "Authentication Error" ) );
        emit finished();
        return;
    }

    m_sessionId = element.text();
    m_authenticated = true;

    emit loginSuccessful();
    emit finished();
}

int
AmpacheAccountLogin::getVersion( const QDomDocument& doc ) const
{
    DEBUG_BLOCK

    QDomElement root = doc.firstChildElement("root");
    //is this an error?
    QDomElement error = root.firstChildElement("error");
    //find status code:
    QDomElement version = root.firstChildElement("version");

    // It's OK if we get a null response from the version, that just means we're dealing with an older version
    if( !error.isNull() )
    {
        // Default the Version down if it didn't work
        debug() << "getVersion error: " << error.text();
        return 100000;
    }
    else if( !version.isNull() )
    {
        debug() << "getVersion returned: " << version.text();
        return version.text().toInt();
    }
    else
    {
        debug() << "getVersion no version";
        return 0;
    }
}

bool
AmpacheAccountLogin::generalVerify( const QDomDocument& doc, NetworkAccessManagerProxy::Error e )
{
    Q_ASSERT( m_lastRequest );

    if( m_lastRequest->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt() != 200 )
    {
        debug() << "server response code:" <<
            m_lastRequest->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt() <<
            m_lastRequest->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString();
        // KMessageBox::error( qobject_cast<QWidget*>(parent()), domError.text(), i18n( "Authentication Error" ) );
        emit finished();
        return false;
    }

    if( e.code != QNetworkReply::NoError )
    {
        debug() << "authenticate Error:" << e.description;
        emit finished();
        return false;
    }

    QDomElement root = doc.firstChildElement("root");
    QDomElement error = root.firstChildElement("error");

    if( !error.isNull() )
    {
        // Default the Version down if it didn't work
        debug() << "generalVerify error: " << error.text();
        KMessageBox::error( qobject_cast<QWidget*>(parent()), error.text(), i18n( "Authentication Error" ) );
        emit finished();
        return false;
    }

    return true;
}

KUrl
AmpacheAccountLogin::getRequestUrl( const QString &action ) const
{
    //lets keep this around for now if we want to allow people to add a service that prompts for stuff
    /* But comment it out since the AmpacheQueryMaker does not do this
    if ( m_server.isEmpty() || m_password.isEmpty() )
    {
        KPasswordDialog dlg( 0 , KPasswordDialog::ShowUsernameLine );
        dlg.setPrompt( i18n( "Enter the server name and a password" ) );
        if( !dlg.exec() )
            return KUrl(); //the user canceled

        m_server = KUrl( dlg.username() ).url();
        m_password = dlg.password();
    }
    */

    QString path = m_server + "/server/xml.server.php";

    if( !path.startsWith("http://") && !path.startsWith("https://") )
        path = "http://" + path;

    KUrl url( path );

    if( !action.isEmpty() )
        url.addQueryItem( "action", action );

    if( !m_username.isEmpty() )
        url.addQueryItem( "user", m_username );

    return url;
}

#include "AmpacheAccountLogin.moc"
