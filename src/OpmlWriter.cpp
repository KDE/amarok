/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "OpmlWriter.h"
#include "core/support/Debug.h"

#include <KUrl>

OpmlWriter::OpmlWriter( const OpmlOutline *rootOutline, QIODevice *device )
    : ThreadWeaver::Job()
    , m_rootOutline( rootOutline )
{
    m_xmlWriter = new QXmlStreamWriter( device );
}

void
OpmlWriter::run()
{
#define _x m_xmlWriter
    _x->setAutoFormatting( true );
    _x->writeStartDocument();
    _x->writeStartElement( "opml" );
    _x->writeAttribute( "version", "2.0" );
    _x->writeStartElement( "head" );
    //root outline is threated special, it's attributes will be the elements of <head>
    QMapIterator<QString, QString> ai( m_rootOutline->attributes() ); //attributesIterator
    while( ai.hasNext() )
    {
        ai.next();
        _x->writeTextElement( ai.key(), ai.value() );
    }
    _x->writeEndElement(); // head
    _x->writeStartElement( "body" );
    foreach( const OpmlOutline *childOutline, m_rootOutline->children() )
        writeOutline( childOutline );
    _x->writeEndDocument(); //implicitly closes all open tags (opml & body)
    emit result( 0 );
}

void
OpmlWriter::writeOutline( const OpmlOutline *outline )
{
    bool hasChildren = outline->children().count() != 0;
    if( hasChildren )
        _x->writeStartElement( "outline" );
    else
        _x->writeEmptyElement( "outline" );
    QMapIterator<QString, QString> ai( outline->attributes() ); // attributesIterator
    while( ai.hasNext() )
    {
        ai.next();
        _x->writeAttribute( ai.key(), ai.value() );
    }

    if( hasChildren )
    {
        foreach( const OpmlOutline *childOutline, outline->children() )
            writeOutline( childOutline );
        _x->writeEndElement(); // outline
    }
}
