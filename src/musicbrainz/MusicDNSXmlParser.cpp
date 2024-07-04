/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#define DEBUG_PREFIX "MusicDNSXmlParser"

#include "MusicDNSXmlParser.h"

#include "core/support/Debug.h"

MusicDNSXmlParser::MusicDNSXmlParser( QByteArray &doc )
                    : QObject()
                    , ThreadWeaver::Job()
                    , m_doc( QStringLiteral("musicdns") )
{
    m_doc.setContent( doc );
}

void
MusicDNSXmlParser::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);
    DEBUG_BLOCK
    QDomElement docElem = m_doc.documentElement();
    parseElement( docElem );
}

void
MusicDNSXmlParser::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void
MusicDNSXmlParser::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}

QStringList
MusicDNSXmlParser::puid()
{
    return ( m_puid.isEmpty() )?m_puid << QStringLiteral("00000000-0000-0000-0000-000000000000"):m_puid;
}

void
MusicDNSXmlParser::parseElement( const QDomElement &e )
{
    QString elementName = e.tagName();
    if( elementName == QStringLiteral("track") )
        parseTrack( e );
    else
        parseChildren( e );
}

void
MusicDNSXmlParser::parseChildren( const QDomElement &e )
{
    QDomNode child = e.firstChild();
    while( !child.isNull() )
    {
        if( child.isElement() )
            parseElement( child.toElement() );
        child = child.nextSibling();
    }
}

void
MusicDNSXmlParser::parseTrack( const QDomElement &e )
{
    QDomNode dNode = e.firstChild();
    QDomElement dElement;

    while( !dNode.isNull() )
    {
        if( dNode.isElement() )
        {
            dElement = dNode.toElement();

            if( dElement.tagName() == QStringLiteral("puid-list") )
                parsePUIDList( dElement );
        }
        dNode = dNode.nextSibling();
    }
}

void
MusicDNSXmlParser::parsePUIDList( const QDomElement &e )
{
    QDomNode dNode = e.firstChild();
    QDomElement dElement;

    while( !dNode.isNull() )
    {
        if( dNode.isElement() )
        {
            dElement = dNode.toElement();

            if( dElement.tagName() == QStringLiteral("puid") )
                parsePUID( dElement );
        }
        dNode = dNode.nextSibling();
    }
}

void
MusicDNSXmlParser::parsePUID( const QDomElement &e )
{
    if( e.hasAttribute( QStringLiteral("id") ) )
    {
        QString id = e.attribute( QStringLiteral("id") );
        if( id.isEmpty() )
            return;
        m_puid << id;
    }
}
