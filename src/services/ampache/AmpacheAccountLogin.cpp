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

#include <QCryptographicHash>
#include <QDomDocument>
#include <QNetworkRequest>
#include <QUrlQuery>

#include <KLocalizedString>
#include <KPasswordDialog>
#include <KMessageBox>


AmpacheAccountLogin::AmpacheAccountLogin( const QUrl& url, const QString& username, const QString& password, QWidget* parent )
    : QObject(parent)
    , m_authenticated( false )
    , m_server( url )
    , m_username( username )
    , m_password( password )
    , m_authRequest( nullptr )
    , m_pingRequest( nullptr )
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
    QUrl url = getRequestUrl( QStringLiteral("ping") );

    debug() << "Verifying Ampache Version Using: " << url.url();

    m_pingRequest = The::networkAccessManager()->getData( url, this, &AmpacheAccountLogin::authenticate );

    if( !m_pingRequest )
        Q_EMIT finished();
}

void
AmpacheAccountLogin::authenticate( const QUrl &requestUrl, const QByteArray &data, const NetworkAccessManagerProxy::Error &e )
{
    if( !m_pingRequest )
        return;

    DEBUG_BLOCK
    Q_UNUSED( requestUrl );

    QDomDocument doc;
    doc.setContent( data );

    if( !generalVerify( m_pingRequest, doc, e ) )
        return;

    // so lets figure out what we got here:
    debug() << "Version reply: " << data;
    int version = getVersion( doc );

    QUrl url = getRequestUrl( QStringLiteral("handshake") );
    QUrlQuery query( url );
    QString timestamp = QString::number( QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000 );
    QString passPhrase;

    // We need to use different authentication strings depending on the version of ampache
    if( version > 350000 )
    {
        debug() << "New Password Scheme " << version;
        query.addQueryItem( QStringLiteral("version"), QStringLiteral("350001") );

        QCryptographicHash sha256Hash( QCryptographicHash::Sha256 );
        sha256Hash.addData( m_password.toUtf8() );
        QString hashedPassword = sha256Hash.result().toHex();

        QString rawHandshake = timestamp + hashedPassword;
        sha256Hash.reset();
        sha256Hash.addData( rawHandshake.toUtf8() );

        passPhrase = sha256Hash.result().toHex();

    }
    else
    {
        debug() << "Version Older than 35001 Generated MD5 Auth " << version;

        QString rawHandshake = timestamp + m_password;
        QCryptographicHash md5Hash( QCryptographicHash::Md5 );

        md5Hash.addData( rawHandshake.toUtf8() );
        passPhrase = md5Hash.result().toHex();
    }

    query.addQueryItem( QStringLiteral("timestamp"), timestamp );
    query.addQueryItem( QStringLiteral("auth"), passPhrase );
    url.setQuery( query );

    debug() << "Authenticating with string: " << url.url() << passPhrase;

    // TODO: Amarok::Logger::newProgressOperation( m_xmlDownloadJob, i18n( "Authenticating with Ampache" ) );
    m_authRequest = The::networkAccessManager()->getData( url, this, &AmpacheAccountLogin::authenticationComplete );

    if( !m_authRequest )
        Q_EMIT finished();
}

void AmpacheAccountLogin::authenticationComplete( const QUrl &requestUrl, const QByteArray &data, const NetworkAccessManagerProxy::Error &e )
{
    Q_UNUSED( requestUrl );

    if( !m_authRequest )
        return;

    DEBUG_BLOCK

    QDomDocument doc;
    doc.setContent( data );

    if( !generalVerify( m_authRequest, doc, e ) )
        return;

    // so lets figure out what we got here:
    debug() << "Authentication reply: " << data;
    QDomElement root = doc.firstChildElement(QStringLiteral("root"));

    //find status code:
    QDomElement element = root.firstChildElement(QStringLiteral("auth"));
    if( element.isNull() )
    {
        // Default the Version down if it didn't work
        debug() << "authenticationComplete failed";
        KMessageBox::error( qobject_cast<QWidget*>(parent()),
                            i18n( "Authentication failed." ),
                            i18n( "Authentication Error" ) );
        Q_EMIT finished();
        return;
    }

    m_sessionId = element.text();
    m_authenticated = true;

    Q_EMIT loginSuccessful();
    Q_EMIT finished();
}

int
AmpacheAccountLogin::getVersion( const QDomDocument& doc ) const
{
    DEBUG_BLOCK

    QDomElement root = doc.firstChildElement(QStringLiteral("root"));
    //is this an error?
    QDomElement error = root.firstChildElement(QStringLiteral("error"));
    //find status code:
    QDomElement version = root.firstChildElement(QStringLiteral("version"));

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
AmpacheAccountLogin::generalVerify( QNetworkReply *reply, const QDomDocument& doc, const NetworkAccessManagerProxy::Error &e )
{
    Q_ASSERT( reply );

    if( reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt() != 200 )
    {
        debug() << "server response code:" <<
            reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt() <<
            reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString();

        Q_EMIT finished();
        return false;
    }

    if( e.code != QNetworkReply::NoError )
    {
        debug() << "authenticate Error:" << e.description;
        Q_EMIT finished();
        return false;
    }

    QDomElement root = doc.firstChildElement(QStringLiteral("root"));
    QDomElement error = root.firstChildElement(QStringLiteral("error"));

    if( !error.isNull() )
    {
        // Default the Version down if it didn't work
        debug() << "generalVerify error: " << error.text();
        KMessageBox::error( qobject_cast<QWidget*>(parent()), error.text(), i18n( "Authentication Error" ) );
        Q_EMIT finished();
        return false;
    }

    return true;
}

QUrl
AmpacheAccountLogin::getRequestUrl( const QString &action ) const
{
    //lets keep this around for now if we want to allow people to add a service that prompts for stuff
    /* But comment it out since the AmpacheQueryMaker does not do this
    if ( m_server.isEmpty() || m_password.isEmpty() )
    {
        KPasswordDialog dlg( 0 , KPasswordDialog::ShowUsernameLine );
        dlg.setPrompt( i18n( "Enter the server name and a password" ) );
        if( !dlg.exec() )
            return QUrl(); //the user canceled

        m_server = QUrl( dlg.username() ).url();
        m_password = dlg.password();
    }
    */

    QUrl url = m_server;
    url.setPath( m_server.path() + QStringLiteral("/server/xml.server.php") );
    QString scheme = m_server.scheme();

    if( scheme != QStringLiteral("http") && scheme != QStringLiteral("https") )
        url.setScheme( QStringLiteral("http") );

    QUrlQuery query( m_server );

    if( !action.isEmpty() )
        query.addQueryItem( QStringLiteral("action"), action );

    if( !m_username.isEmpty() && action != QStringLiteral("ping") )
        query.addQueryItem( QStringLiteral("user"), m_username );

    if( !m_sessionId.isEmpty() && action == QStringLiteral("ping") )
        query.addQueryItem( QStringLiteral("auth"), m_sessionId );

    url.setQuery( query );

    return url;
}

