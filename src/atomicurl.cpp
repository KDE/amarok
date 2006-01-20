/*
  Copyright (c) 2005 Gábor Lehel <illissius@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include <qdeepcopy.h>
#include <kurl.h>
#include "debug.h"

#include "atomicurl.h"

AtomicURL::AtomicURL() { }

AtomicURL::AtomicURL( const AtomicURL &other )
{
    m_beginning = other.m_beginning;
    m_directory = other.m_directory;
    m_filename = other.m_filename;
}

AtomicURL::AtomicURL( const KURL &url )
{
    if( url.isEmpty() )
        return;

    QString s = url.protocol() + "://";
    QString host = url.host();
    if( url.hasUser() )
    {
        s += url.user();
        host.prepend("@");
    }
    if( url.hasPass() )
        s += ":" + url.pass();
    if( url.port() )
        host += QString(":") + QString::number( url.port() );

    m_beginning = s + host;
    m_directory = url.directory();
    QString f = url.fileName() + url.query();
    if( url.hasRef() )
        f += QString("#") + url.ref();
    m_filename = f;
    /*if (url != this->url())
    {
        debug() << "from: " << url << endl;
        debug() << "to:   " << this->url() << endl;
    }*/
}

AtomicURL::~AtomicURL() { }

AtomicURL &AtomicURL::operator=( const AtomicURL &other )
{
    m_beginning = other.m_beginning;
    m_directory = other.m_directory;
    m_filename = other.m_filename;
    return *this;
}

bool AtomicURL::operator==( const AtomicURL &other ) const
{
    return m_filename  == other.m_filename
        && m_directory == other.m_directory
        && m_beginning == other.m_beginning;
}

KURL AtomicURL::url() const
{
    if( isEmpty() )
        return KURL();
    QString s = m_beginning;
    if( !m_directory->isEmpty() || !m_filename.isEmpty() )
    {
        s += "/foo/";
    }
    KURL u( s );
    u.setPath( path() );
    return u;
}

KURL AtomicURL::deepCopy() const
{
    KURL u( m_beginning.deepCopy() );
    u.setDirectory( m_directory.deepCopy() );
    u.setFileName( QDeepCopy<QString>( m_filename ) );
    return u;
}

bool AtomicURL::isEmpty() const
{
    return m_beginning->isEmpty()
    && m_directory->isEmpty()
    && m_filename.isEmpty();
}

void AtomicURL::setPath( const QString &path )
{
    KURL url;
    url.setPath( path );
    *this = url;
}

QString AtomicURL::path() const
{
    if( !m_filename.isEmpty() && !m_directory->endsWith("/") )
        return m_directory + "/" + m_filename;
    return m_directory + m_filename;
}

QString AtomicURL::fileName() const { return m_filename; }
QString AtomicURL::directory() const { return m_directory; }
