/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "OpmlDirectoryXmlParser.h"

#include "Amarok.h"
#include "Debug.h"
#include "statusbar/StatusBar.h"

#include <QDomDocument>
#include <QFile>

#include <KLocale>
#include <threadweaver/Job.h>


OpmlOutline::OpmlOutline( OpmlOutline *parent )
        : m_parent( parent )
{
    DEBUG_BLOCK
}

using namespace Meta;

OpmlDirectoryXmlParser::OpmlDirectoryXmlParser( const QString &filename )
        : ThreadWeaver::Job()
        , n_numberOfTransactions ( 0 )
        , n_maxNumberOfTransactions ( 5000 )
{
    DEBUG_BLOCK
    m_sFileName = filename;
    albumTags.clear();
    m_dbHandler = new OpmlDirectoryDatabaseHandler();
    m_currentCategoryId = -1;
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
}

OpmlDirectoryXmlParser::~OpmlDirectoryXmlParser()
{
    DEBUG_BLOCK
    delete m_dbHandler;
}

void
OpmlDirectoryXmlParser::run()
{
    readConfigFile( m_sFileName );
}

void
OpmlDirectoryXmlParser::completeJob( )
{
    The::statusBar()->longMessage(
                   i18ncp( "This string is the first part of the following example phrase: Podcast Directory update complete. Added 4 feeds in 6 categories.", "Podcast Directory update complete. Added 1 feed in ", "Podcast Directory update complete. Added %1 feeds in ", m_nNumberOfFeeds)
            + i18ncp( "This string is the second part of the following example phrase: Podcast Directory update complete. Added 4 feeds in 6 categories.", "1 category.", "%1 categories.", m_nNumberOfCategories),
        StatusBar::Information );


    debug() << "OpmlDirectoryXmlParser: total number of albums: " << m_nNumberOfCategories;
    debug() << "OpmlDirectoryXmlParser: total number of tracks: " << m_nNumberOfFeeds;
    emit doneParsing();
    deleteLater();
}

void
OpmlDirectoryXmlParser::readConfigFile( const QString &filename )
{
    DEBUG_BLOCK
    m_nNumberOfFeeds = 0;
    m_nNumberOfCategories = 0;

    QDomDocument doc( "opml" );

    if ( !QFile::exists( filename ) )
    {
        debug() << "Opml file does not exist";
        return;
    }

    QFile file( filename );
    if ( !file.open( QIODevice::ReadOnly ) ) {
        debug() << "OpmlDirectoryXmlParser::readConfigFile error reading file";
        return ;
    }
    if ( !doc.setContent( &file ) )
    {
        debug() << "OpmlDirectoryXmlParser::readConfigFile error parsing file";
        file.close();
        return ;
    }
    file.close();

    QFile::remove( filename );

    m_dbHandler->destroyDatabase();
    m_dbHandler->createDatabase();

    //run through all the elements
    QDomElement docElem = doc.documentElement();

    m_dbHandler->begin(); //start transaction (MAJOR speedup!!)
    debug() << "begin parsing content";
    parseElement( docElem );
    debug() << "finishing transaction";
    m_dbHandler->commit(); //complete transaction

    //completeJob is called by ThreadManager
}

OpmlOutline*
OpmlDirectoryXmlParser::parseOutlineElement( const QDomElement &e )
{
    if( e.tagName() != "outline" )
        return 0;

    OpmlOutline *outline = new OpmlOutline();

    QDomNamedNodeMap attributes = e.attributes();
    for( int i = 0; i< attributes.length(); i++ )
    {
        QDomAttr attribute = attributes.item( i ).toAttr();
        outline->addAttribute( attribute.name(), attribute.value() );
    }

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

void
OpmlDirectoryXmlParser::parseElement( const  QDomElement &e )
{
    QString sElementName = e.tagName();

    if( sElementName == "outline" )
    {
        if ( e.hasChildNodes() )
            parseCategory( e );
        else
            parseFeed( e );
    }
    else
        parseChildren( e );
}

void
OpmlDirectoryXmlParser::parseChildren( const  QDomElement &e )
{
    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {
        if ( n.isElement() )
            parseElement( n.toElement() );

        n = n.nextSibling();
    }
}


void OpmlDirectoryXmlParser::parseCategory( const  QDomElement &e)
{
    m_nNumberOfCategories++;

    QString name = e.attribute( "text", "Unknown" );
    ServiceAlbumPtr currentCategory = ServiceAlbumPtr( new OpmlDirectoryCategory( name ) );

    m_currentCategoryId = m_dbHandler->insertAlbum( currentCategory );
    countTransaction();

    parseChildren( e );
}

void OpmlDirectoryXmlParser::parseFeed( const QDomElement &e )
{
    m_nNumberOfFeeds++;

    QString name = e.attribute( "text", "Unknown" );
    QString url = e.attribute( "url", "" );

    OpmlDirectoryFeedPtr currentFeed = OpmlDirectoryFeedPtr( new OpmlDirectoryFeed( name ) );
    currentFeed->setAlbumId( m_currentCategoryId );
    currentFeed->setUidUrl( url );

    m_dbHandler->insertTrack( ServiceTrackPtr::dynamicCast( currentFeed ) );
    
    countTransaction();
}


void OpmlDirectoryXmlParser::countTransaction()
{
    n_numberOfTransactions++;
    if ( n_numberOfTransactions >= n_maxNumberOfTransactions )
    {
        m_dbHandler->commit();
        m_dbHandler->begin();
        n_numberOfTransactions = 0;
    }
}

#include "OpmlDirectoryXmlParser.moc"

