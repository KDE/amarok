/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "OpmlParser.h"

#include "Amarok.h"
#include "Debug.h"
#include "statusbar/StatusBar.h"

#include <QDomDocument>
#include <QFile>

#include <KLocale>
#include <threadweaver/Job.h>


OpmlOutline::OpmlOutline( OpmlOutline *parent )
        : m_parent( parent )
        , m_hasChildren( false )
{
    DEBUG_BLOCK
}

using namespace Meta;

const QString OpmlParser::OPML_MIME = "text/x-opml+xml";

OpmlParser::OpmlParser( const QString &filename )
        : ThreadWeaver::Job()
{
    DEBUG_BLOCK
    m_sFileName = filename;
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SIGNAL( doneParsing() ) );
}

OpmlParser::~OpmlParser()
{
    DEBUG_BLOCK
}

void
OpmlParser::run()
{
    readConfigFile( m_sFileName );
}

void
OpmlParser::readConfigFile( const QString &filename )
{
    DEBUG_BLOCK

    QDomDocument doc( "opml" );

    if ( !QFile::exists( filename ) )
    {
        debug() << "Opml file does not exist";
        return;
    }

    QFile file( filename );
    if ( !file.open( QIODevice::ReadOnly ) ) {
        debug() << "OpmlParser::readConfigFile error reading file";
        return ;
    }
    if ( !doc.setContent( &file ) )
    {
        debug() << "OpmlParser::readConfigFile error parsing file";
        file.close();
        return ;
    }
    file.close();

    //run through all the elements
    QDomElement docElem = doc.documentElement();

    QDomNode bodyNode = docElem.namedItem( "body" );
    if( bodyNode.isNull() || !bodyNode.isElement() )
        return; //TODO: emit parsing element

    debug() << "begin parsing content";
    parseOpmlBody( bodyNode.toElement() );
    debug() << "finishing transaction";

    emit( doneParsing() );
}

void
OpmlParser::parseOpmlBody( const QDomElement &e )
{
    if( e.tagName() != "body" )
        return; //TODO: emit parsing element

    QDomElement node = e.firstChildElement( "outline" );
    while( !node.isNull() )
    {
        OpmlOutline *outline = parseOutlineElement( node );
        m_rootOutlines << outline;
        emit( outlineParsed( outline ) );
        node = node.nextSiblingElement( "outline" );
    }
}

OpmlOutline*
OpmlParser::parseOutlineElement( const QDomElement &e )
{
    if( e.tagName() != "outline" )
        return 0;

    OpmlOutline *outline = new OpmlOutline();

    QDomNamedNodeMap attributes = e.attributes();
    for( unsigned int i = 0; i < attributes.length(); i++ )
    {
        QDomAttr attribute = attributes.item( i ).toAttr();
        outline->addAttribute( attribute.name(), attribute.value() );
    }
    outline->setHasChildren( e.hasChildNodes() );

    emit( outlineParsed( outline ) );

    QDomNodeList childNodes = e.childNodes();
    for( int i = 0; i < childNodes.count(); i++ )
    {
        QDomNode node = childNodes.item( i );
        if( !node.isElement() )
            continue;
        const QDomElement &element = node.toElement();
        outline->addChild( parseOutlineElement( element ) );
    }

    return outline;
}
