/***************************************************************************
 *   Copyright (C) 2009 Sven Krohlas <sven@getamarok.com>                  *
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

#ifndef KURLEXPORTER_H
#define KURLEXPORTER_H

#include <QByteArray>
#include <QObject>
#include <QScriptable>
#include <QString>

#include <KUrl>

class KUrlPrototype : public QObject, protected QScriptable
{
    Q_OBJECT

    Q_PROPERTY( QString authority WRITE setAuthority READ authority )
    Q_PROPERTY( QByteArray encodedFragment WRITE setEncodedFragment READ encodedFragment )
    Q_PROPERTY( QByteArray encodedHost WRITE setEncodedHost READ encodedHost )
    Q_PROPERTY( QByteArray encodedPassword WRITE setEncodedPassword READ encodedPassword )
    Q_PROPERTY( QByteArray encodedPath WRITE setEncodedPath READ encodedPath )
    Q_PROPERTY( QByteArray encodedUserName WRITE setEncodedUserName READ encodedUserName )
    Q_PROPERTY( QString fragment WRITE setFragment READ fragment )
    Q_PROPERTY( QString host WRITE setHost READ host )
    Q_PROPERTY( QString password WRITE setPassword READ password )
    Q_PROPERTY( QString path WRITE setPath READ path )
    Q_PROPERTY( int port WRITE setPort READ port )
    Q_PROPERTY( QString scheme WRITE setScheme READ scheme )
    Q_PROPERTY( QString url WRITE setUrl )
    Q_PROPERTY( QString userInfo WRITE setUserInfo READ userInfo )
    Q_PROPERTY( QString userName WRITE setUserName READ userName )

    public:
        KUrlPrototype();
        ~KUrlPrototype();

    private:
        QString authority() const;
        QByteArray encodedFragment() const;
        QByteArray encodedHost() const;
        QByteArray encodedPassword() const;
        QByteArray encodedPath() const;
        QByteArray encodedQuery() const;
        QByteArray encodedUserName() const;
        QString fragment() const;
        QString host() const;
        QString password() const;
        QString path() const;
        int port() const;
        QString scheme() const;
        void setAuthority( const QString & authority );
        void setEncodedFragment( const QByteArray & fragment );
        void setEncodedHost( const QByteArray & host );
        void setEncodedPassword( const QByteArray & password );
        void setEncodedPath( const QByteArray & path );
        void setEncodedQuery( const QByteArray & query );
        void setEncodedUrl( const QByteArray & encodedUrl );
        void setEncodedUserName( const QByteArray & userName );
        void setFragment( const QString & fragment );
        void setHost( const QString & host );
        void setPassword( const QString & password );
        void setPath( const QString & path );
        void setPort( int port );
        void setScheme( const QString & scheme );
        void setUrl( const QString & url );
        void setUserInfo( const QString & userInfo );
        void setUserName( const QString & userName );
        QString userInfo() const;
        QString userName() const;
};

#endif // KURLEXPORTER_H
