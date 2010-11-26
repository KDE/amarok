/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
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

#include "XmlQueryWriter.h"
#include "core/meta/support/MetaConstants.h"

#include "core/support/Debug.h"

#include <QTextStream>

using namespace Collections;

XmlQueryWriter::XmlQueryWriter( QueryMaker* qm, QDomDocument doc )
    : m_qm( qm ), m_doc( doc ), m_andorLevel( 0 )
{
    m_element = m_doc.createElement( "query" );
    m_element.setAttribute( "version", "1.0" );

    m_filterElement = m_doc.createElement( "filters" );
    m_element.appendChild( m_filterElement );

    // connect up the signals
    connect( m_qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SIGNAL( newResultReady( QString, Meta::TrackList ) ), Qt::DirectConnection );
    connect( m_qm, SIGNAL( newResultReady( QString, Meta::ArtistList ) ), this, SIGNAL( newResultReady( QString, Meta::ArtistList ) ), Qt::DirectConnection );
    connect( m_qm, SIGNAL( newResultReady( QString, Meta::AlbumList ) ), this, SIGNAL( newResultReady( QString, Meta::AlbumList ) ), Qt::DirectConnection );
    connect( m_qm, SIGNAL( newResultReady( QString, Meta::GenreList ) ), this, SIGNAL( newResultReady( QString, Meta::GenreList ) ), Qt::DirectConnection );
    connect( m_qm, SIGNAL( newResultReady( QString, Meta::ComposerList ) ), this, SIGNAL( newResultReady( QString, Meta::ComposerList ) ), Qt::DirectConnection );
    connect( m_qm, SIGNAL( newResultReady( QString, Meta::YearList ) ), this, SIGNAL( newResultReady( QString, Meta::YearList ) ), Qt::DirectConnection );
    connect( m_qm, SIGNAL( newResultReady( QString, QStringList ) ), this, SIGNAL( newResultReady( QString, QStringList ) ), Qt::DirectConnection );
    connect( m_qm, SIGNAL( newResultReady( QString, Meta::DataList ) ), this, SIGNAL( newResultReady( QString, Meta::DataList ) ), Qt::DirectConnection );
    connect( m_qm, SIGNAL( queryDone() ), this, SIGNAL( queryDone() ) );
}


XmlQueryWriter::~XmlQueryWriter()
{
    delete m_qm;
}


QString
XmlQueryWriter::getXml( int indent )
{
    QString raw;
    QTextStream rawStream( &raw );
    m_element.save( rawStream, indent );

    return raw;
}

QDomElement
XmlQueryWriter::getDomElement() const
{
    return m_element;
}

QueryMaker*
XmlQueryWriter::getEmbeddedQueryMaker() const
{
    return m_qm;
}

void
XmlQueryWriter::run()
{
    m_qm->run();
}


void
XmlQueryWriter::abortQuery()
{
    m_qm->abortQuery();
}


int
XmlQueryWriter::resultCount() const
{
    return m_qm->resultCount();
}

QueryMaker*
XmlQueryWriter::setQueryType( QueryType type )
{
    switch( type ) {
    case QueryMaker::Track:
        insertRetValue( "track" );
        m_qm->setQueryType( QueryMaker::Track );
        return this;

    case QueryMaker::Artist:
        insertRetValue( "artist" );
        m_qm->setQueryType( QueryMaker::Artist );
        return this;

    case QueryMaker::Album:
        insertRetValue( "album" );
        m_qm->setQueryType( QueryMaker::Album );
        return this;
    case QueryMaker::AlbumArtist:
        insertRetValue( "albumartist" );
        m_qm->setQueryType( QueryMaker::AlbumArtist );
        return this;
    case QueryMaker::Genre:
        insertRetValue( "genre" );
        m_qm->setQueryType( QueryMaker::Genre );
        return this;

    case QueryMaker::Composer:
        insertRetValue( "composer" );
        m_qm->setQueryType( QueryMaker::Composer );
        return this;

    case QueryMaker::Year:
        insertRetValue( "year" );
        m_qm->setQueryType( QueryMaker::Year );
        return this;

    case QueryMaker::Custom:
        // TODO
        m_qm->setQueryType( QueryMaker::Custom );
        return this;

    case QueryMaker::Label:
        insertRetValue( "label" );
        m_qm->setQueryType( QueryMaker::Label );
        return this;

    case QueryMaker::None:
        return this;
    }
    return this;
}

QueryMaker*
XmlQueryWriter::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
{
    QDomElement e = m_doc.createElement( "returnResultsAsDataPairs" );
    m_element.appendChild( e );

    m_qm->setReturnResultAsDataPtrs( resultAsDataPtrs );
    return this;
}


QueryMaker*
XmlQueryWriter::addReturnValue( qint64 value )
{
    // TODO
    m_qm->addReturnValue( value );
    return this;
}

QueryMaker*
XmlQueryWriter::addReturnFunction( ReturnFunction function, qint64 value )
{
    // TODO
    m_qm->addReturnFunction( function, value );
    return this;
}

QueryMaker*
XmlQueryWriter::orderBy( qint64 value, bool descending )
{
    QDomElement e = m_doc.createElement( "order" );
    e.setAttribute( "field", Meta::nameForField( value ) );
    e.setAttribute( "value", descending ? "descending" : "ascending" );
    m_element.appendChild( e );

    m_qm->orderBy( value, descending );
    return this;
}

QueryMaker*
XmlQueryWriter::orderByRandom()
{
    QDomElement e = m_doc.createElement( "order" );
    e.setAttribute( "value", "random" );
    m_element.appendChild( e );

    m_qm->orderByRandom();
    return this;

}

QueryMaker*
XmlQueryWriter::includeCollection( const QString &collectionId )
{
    QDomElement e = m_doc.createElement( "includeCollection" );
    e.setAttribute( "id", collectionId );
    m_element.appendChild( e );

    m_qm->includeCollection( collectionId );
    return this;
}


QueryMaker*
XmlQueryWriter::excludeCollection( const QString &collectionId )
{
    QDomElement e = m_doc.createElement( "excludeElement" );
    e.setAttribute( "id", collectionId );
    m_element.appendChild( e );

    m_qm->includeCollection( collectionId );
    return this;
}

QueryMaker*
XmlQueryWriter::addMatch( const Meta::TrackPtr &track )
{
    m_qm->addMatch( track );
    return this;
}

QueryMaker*
XmlQueryWriter::addMatch( const Meta::ArtistPtr &artist )
{
    m_qm->addMatch( artist );
    return this;
}

QueryMaker*
XmlQueryWriter::addMatch( const Meta::AlbumPtr &album )
{
    m_qm->addMatch( album );
    return this;
}

QueryMaker*
XmlQueryWriter::addMatch( const Meta::ComposerPtr &composer )
{
    m_qm->addMatch( composer );
    return this;
}

QueryMaker*
XmlQueryWriter::addMatch( const Meta::GenrePtr &genre )
{
    m_qm->addMatch( genre );
    return this;
}

QueryMaker*
XmlQueryWriter::addMatch( const Meta::YearPtr &year )
{
    m_qm->addMatch( year );
    return this;
}

QueryMaker*
XmlQueryWriter::addMatch( const Meta::LabelPtr &label )
{
    m_qm->addMatch( label );
    return this;
}

QueryMaker*
XmlQueryWriter::addMatch( const Meta::DataPtr &data )
{
    m_qm->addMatch( data );
    return this;
}

QueryMaker*
XmlQueryWriter::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    DEBUG_BLOCK
    m_filterElement.appendChild( xmlForFilter(m_doc, false, value, filter ));

    m_qm->addFilter( value, filter, matchBegin, matchEnd );
    return this;
}

QueryMaker*
XmlQueryWriter::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    m_filterElement.appendChild( xmlForFilter(m_doc, true, value, filter ));

    m_qm->excludeFilter( value, filter, matchBegin, matchEnd );
    return this;
}

QueryMaker*
XmlQueryWriter::addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    m_filterElement.appendChild( xmlForFilter(m_doc, false, value, filter, compare ));

    m_qm->addNumberFilter( value, filter, compare );
    return this;
}

QueryMaker*
XmlQueryWriter::excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    m_filterElement.appendChild( xmlForFilter(m_doc, true, value, filter, compare ));

    m_qm->excludeNumberFilter( value, filter, compare );
    return this;
}


QueryMaker*
XmlQueryWriter::limitMaxResultSize( int size )
{
    QDomElement e = m_doc.createElement( "limit" );
    e.setAttribute( "value", size );
    m_element.appendChild( e );

    m_qm->limitMaxResultSize( size );
    return this;
}

QueryMaker*
XmlQueryWriter::setAlbumQueryMode( AlbumQueryMode mode )
{
    // there can be only one
    m_element.removeChild( m_element.lastChildElement( "onlyCompilations" ) );
    m_element.removeChild( m_element.lastChildElement( "onlyNormalAlbums" ) );

    if( mode == OnlyCompilations )
    {
        QDomElement e = m_doc.createElement( "onlyCompilations" );
        m_element.appendChild( e );
    }
    if( mode == OnlyNormalAlbums )
    {
        QDomElement e = m_doc.createElement( "onlyNormalAlbums" );
        m_element.appendChild( e );
    }

    m_qm->setAlbumQueryMode( mode );
    return this;
}

QueryMaker *
XmlQueryWriter::setArtistQueryMode( QueryMaker::ArtistQueryMode mode )
{
    m_element.removeChild( m_element.lastChildElement( "onlyTrackArtists" ) );
    m_element.removeChild( m_element.lastChildElement( "onlyAlbumArtists" ) );
    m_element.removeChild( m_element.lastChildElement( "AlbumOrTrackArtists" ) );

    if( mode == TrackArtists )
    {
        QDomElement e = m_doc.createElement( "onlyTrackArtists" );
        m_element.appendChild( e );
    }
    else if( mode == AlbumArtists )
    {
        QDomElement e = m_doc.createElement( "onlyAlbumArtists" );
        m_element.appendChild( e );
    }
    else if( mode == AlbumOrTrackArtists )
    {
        QDomElement e = m_doc.createElement( "AlbumOrTrackArtists" );
        m_element.appendChild( e );
    }

    m_qm->setArtistQueryMode( mode );
    return this;
}

QueryMaker*
XmlQueryWriter::beginAnd()
{
    QDomElement e = m_doc.createElement( "and" );
    m_filterElement.appendChild( e );
    m_filterElement = e;
    m_andorLevel++;
    
    m_qm->beginAnd();
    return this;
}

QueryMaker*
XmlQueryWriter::beginOr()
{
    QDomElement e = m_doc.createElement( "or" );
    m_filterElement.appendChild( e );
    m_filterElement = e;
    m_andorLevel++;
    
    m_qm->beginOr();
    return this;
}

QueryMaker*
XmlQueryWriter::endAndOr()
{
    if( m_andorLevel > 0 )
    {
        m_filterElement = m_filterElement.parentNode().toElement();
        m_andorLevel--;
    }

    m_qm->endAndOr();
    return this;
}


int
XmlQueryWriter::validFilterMask()
{
    return m_qm->validFilterMask();
}

QDomElement
XmlQueryWriter::xmlForFilter( QDomDocument doc, bool exclude, quint64 field, QString value)
{
    QDomElement e = doc.createElement( exclude ? "exclude" : "include" );
    e.setAttribute( "field", Meta::nameForField( field ) );
    e.setAttribute( "value", value );

    return e;
}

QDomElement
XmlQueryWriter::xmlForFilter( QDomDocument doc, bool exclude, quint64 field, quint64 numValue, NumberComparison compare)
{
    QDomElement e = doc.createElement( exclude ? "exclude" : "include" );
    e.setAttribute( "field", Meta::nameForField( field ) );
    e.setAttribute( "value", numValue );
    e.setAttribute( "compare", compareName( compare ) );

    return e;
}

void
XmlQueryWriter::insertRetValue( QString val )
{
    if( m_retvalElement.isNull() )
    {
        m_retvalElement = m_doc.createElement( "returnValues" );
        m_element.appendChild( m_retvalElement );
    }

    QDomElement retval = m_doc.createElement( val );
    m_retvalElement.appendChild( retval );
}

QString
XmlQueryWriter::compareName( QueryMaker::NumberComparison compare )
{
    if( compare == LessThan )
        return "less";
    else if( compare == GreaterThan )
        return "greater";
    else
        return "equals";
}

