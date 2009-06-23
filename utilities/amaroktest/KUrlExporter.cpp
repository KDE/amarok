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

/** Very basic KUrl prototype, only set/get functionality from QUrl attributes atm */

#include "KUrlExporter.h"
#include "KUrlExporter.moc"

#include <QtScript>

#define GET_URL KUrl url = qscriptvalue_cast<KUrl>( thisObject() );

KUrlPrototype::KUrlPrototype()
{
}

KUrlPrototype::~KUrlPrototype()
{
}

QString
KUrlPrototype::authority() const
{
    GET_URL
    return url.authority();
}

QByteArray
KUrlPrototype::encodedFragment() const
{
    GET_URL
    return url.encodedFragment();
}

QByteArray
KUrlPrototype::encodedHost() const
{
    GET_URL
    return url.encodedHost();
}

QByteArray
KUrlPrototype::encodedPassword() const
{
    GET_URL
    return url.encodedPassword();
}

QByteArray
KUrlPrototype::encodedPath() const
{
    GET_URL
    return url.encodedPath();
}

QByteArray
KUrlPrototype::encodedQuery() const
{
    GET_URL
    return url.encodedQuery();
}

QByteArray
KUrlPrototype::encodedUserName() const
{
    GET_URL
    return url.encodedUserName();
}

QString
KUrlPrototype::fragment() const
{
    GET_URL
    return url.fragment();
}

QString
KUrlPrototype::host() const
{
    GET_URL
    return url.host();
}

QString
KUrlPrototype::password() const
{
    GET_URL
    return url.password();
}

QString
KUrlPrototype::path() const
{
    GET_URL
    return url.path();
}

int
KUrlPrototype::port() const
{
    GET_URL
    return url.port();
}

QString
KUrlPrototype::scheme() const
{
    GET_URL
    return url.scheme();
}

void
KUrlPrototype::setAuthority( const QString & authority )
{
    GET_URL
    url.setAuthority( authority );
}

void
KUrlPrototype::setEncodedFragment( const QByteArray & fragment )
{
    GET_URL
    url.setEncodedFragment( fragment );
}

void
KUrlPrototype::setEncodedHost( const QByteArray & host )
{
    GET_URL
    url.setEncodedHost( host );
}

void
KUrlPrototype::setEncodedPassword( const QByteArray & password )
{
    GET_URL
    url.setEncodedPassword( password );
}

void
KUrlPrototype::setEncodedPath( const QByteArray & path )
{
    GET_URL
    url.setEncodedPassword( path );
}

void
KUrlPrototype::setEncodedQuery( const QByteArray & query )
{
    GET_URL
    url.setEncodedQuery( query );
}

void
KUrlPrototype::setEncodedUrl( const QByteArray & encodedUrl )
{
    GET_URL
    url.setEncodedUrl( encodedUrl );
}

void
KUrlPrototype::setEncodedUserName( const QByteArray & userName )
{
    GET_URL
    url.setEncodedUserName( userName );
}

void
KUrlPrototype::setFragment( const QString & fragment )
{
    GET_URL
    url.setFragment( fragment );
}

void
KUrlPrototype::setHost( const QString & host )
{
    GET_URL
    url.setHost( host );
}

void
KUrlPrototype::setPassword( const QString & password )
{
    GET_URL
    url.setPassword( password );
}

void
KUrlPrototype::setPath( const QString & path )
{
    GET_URL
    url.setPath( path );
}

void
KUrlPrototype::setPort( int port )
{
    GET_URL
    url.setPort( port );
}

void
KUrlPrototype::setScheme( const QString & scheme )
{
    GET_URL
    url.setScheme( scheme );
}

void
KUrlPrototype::setUrl( const QString & myUrl ) /** different parameter name to avoid collision with GET_URL macro */
{
    GET_URL
    url.setUrl( myUrl );
}

void
KUrlPrototype::setUserInfo( const QString & userInfo )
{
    GET_URL
    url.setUserInfo( userInfo );
}

void
KUrlPrototype::setUserName( const QString & userName )
{
    GET_URL
    url.setUserName( userName );
}

QString
KUrlPrototype::userInfo() const
{
    GET_URL
    return url.userInfo();
}

QString
KUrlPrototype::userName() const
{
    GET_URL
    return url.userName();
}

#undef GET_URL
