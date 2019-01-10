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


#ifndef AMPACHEACCOUNTLOGIN_H
#define AMPACHEACCOUNTLOGIN_H

#include "NetworkAccessManagerProxy.h"

#include <QDomDocument>
#include <QObject>
#include <QUrl>

class QNetworkReply;

#ifdef MAKE_AMPACHE_ACCOUNT_LOGIN_LIB
#define AMPACHE_ACCOUNT_EXPORT Q_DECL_EXPORT
#else
#define AMPACHE_ACCOUNT_EXPORT Q_DECL_IMPORT
#endif

class AMPACHE_ACCOUNT_EXPORT AmpacheAccountLogin : public QObject
{
    Q_OBJECT
    public:
        AmpacheAccountLogin ( const QUrl& url, const QString& username, const QString& password, QWidget* parent = nullptr );
        ~AmpacheAccountLogin();
        QUrl server() const { return m_server; }
        QString sessionId() const { return m_sessionId; }
        bool authenticated() const { return m_authenticated; }
        void reauthenticate();

    Q_SIGNALS:
        void loginSuccessful(); //!authentication was successful
        void finished(); //!authentication was or was not successful

    private Q_SLOTS:
        void authenticate( const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e );
        void authenticationComplete( const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e );

    private:
        int getVersion( const QDomDocument& doc ) const;

        /** Does general response verification.
            Emits finished if something is fishy.
            @returns true if the check was successful.
        */
        bool generalVerify( QNetworkReply *reply, const QDomDocument& doc, const NetworkAccessManagerProxy::Error &e );

        /** Returns the base url.
            You would need to add query items to use it. */
        QUrl getRequestUrl( const QString &action = QString() ) const;

        bool m_authenticated;
        QUrl m_server;
        QString m_username;
        QString m_password;
        QString m_sessionId;

        QNetworkReply *m_authRequest;
        QNetworkReply *m_pingRequest;
};

#endif // AMPACHEACCOUNTLOGIN_H
