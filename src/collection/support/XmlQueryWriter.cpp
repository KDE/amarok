/***************************************************************************
 * copyright        : (C) 2008 Daniel Caleb Jones <danielcjones@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "XmlQueryWriter.h"

#include <QTextStream>

XmlQueryWriter::XmlQueryWriter( QueryMaker* qm )
    : m_qm( qm ), m_andorLevel( 0 )
{
    m_element.setTagName( "query" );
    m_element.setAttribute( "version", "1.0" );

    m_filterElement.setTagName( "filters" );
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
XmlQueryWriter::getDomElement()
{
    return m_element;
}

QueryMaker*
XmlQueryWriter::getEmededQueryMaker()
{
    return m_qm;
}


QueryMaker*
XmlQueryWriter::reset()
{
    QDomNode child = m_element.firstChild();
    while( !child.isNull() )
    {
        m_element.removeChild( child );
        child = m_element.firstChild();
    }
    
    m_qm->reset();
    return this;
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
XmlQueryWriter::startTrackQuery()
{
    insertRetValue( "track" );
    m_qm->startTrackQuery();
    return this;
}

QueryMaker*
XmlQueryWriter::startArtistQuery()
{
    insertRetValue( "artist" );
    m_qm->startArtistQuery();
    return this;
}

QueryMaker*
XmlQueryWriter::startAlbumQuery()
{
    insertRetValue( "album" );
    m_qm->startAlbumQuery();
    return this;
}

QueryMaker*
XmlQueryWriter::startGenreQuery()
{
    insertRetValue( "genre" );
    m_qm->startGenreQuery();
    return this;
}

QueryMaker*
XmlQueryWriter::startComposerQuery()
{
    insertRetValue( "composer" );
    m_qm->startComposerQuery();
    return this;
}

QueryMaker*
XmlQueryWriter::startYearQuery()
{
    insertRetValue( "year" );
    m_qm->startYearQuery();
    return this;
}

QueryMaker*
XmlQueryWriter::startCustomQuery()
{
    // TODO
    m_qm->startCustomQuery();
    return this;
}

QueryMaker*
XmlQueryWriter::returnResultAsDataPtrs( bool resultAsDataPtrs )
{
    QDomElement e;
    e.setTagName( "returnResultAsDataPtrs" );
    m_element.appendChild( e );

    m_qm->returnResultAsDataPtrs( resultAsDataPtrs );
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
    QDomElement e;
    e.setTagName( "order" );
    e.setAttribute( "field", fieldName( value ) );
    e.setAttribute( "value", descending ? "descending" : "ascending" );
    m_element.appendChild( e );

    m_qm->orderBy( value, descending );
    return this;
}

QueryMaker*
XmlQueryWriter::orderByRandom()
{
    QDomElement e;
    e.setTagName( "order" );
    e.setAttribute( "value", "random" );
    m_element.appendChild( e );

    m_qm->orderByRandom();
    return this;

}

QueryMaker*
XmlQueryWriter::includeCollection( const QString &collectionId )
{
    QDomElement e;
    e.setTagName( "includeCollection" );
    e.setAttribute( "id", collectionId );
    m_element.appendChild( e );

    m_qm->includeCollection( collectionId );
    return this;
}


QueryMaker*
XmlQueryWriter::excludeCollection( const QString &collectionId )
{
    QDomElement e;
    e.setTagName( "excludeCollection" );
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
XmlQueryWriter::addMatch( const Meta::DataPtr &data )
{
    m_qm->addMatch( data );
    return this;
}

QueryMaker*
XmlQueryWriter::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    QDomElement e;
    e.setTagName( "include" );
    e.setAttribute( "field", fieldName( value ) );
    e.setAttribute( "value", filter );
    m_filterElement.appendChild( e );

    m_qm->addFilter( value, filter, matchBegin, matchEnd );    
    return this;
}

QueryMaker*
XmlQueryWriter::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    QDomElement e;
    e.setTagName( "exclude" );
    e.setAttribute( "field", fieldName( value ) );
    e.setAttribute( "value", filter );
    m_filterElement.appendChild( e );

    m_qm->excludeFilter( value, filter, matchBegin, matchEnd );    
    return this;
}

QueryMaker*
XmlQueryWriter::addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    QDomElement e;
    e.setTagName( "include" );
    e.setAttribute( "field", fieldName( value ) );
    e.setAttribute( "value", filter );
    e.setAttribute( "compare", compareName( compare ) );
    m_filterElement.appendChild( e );

    m_qm->addNumberFilter( value, filter, compare );    
    return this;
}

QueryMaker*
XmlQueryWriter::excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    QDomElement e;
    e.setTagName( "exclude" );
    e.setAttribute( "field", fieldName( value ) );
    e.setAttribute( "value", filter );
    e.setAttribute( "compare", compareName( compare ) );
    m_filterElement.appendChild( e );

    m_qm->excludeNumberFilter( value, filter, compare );    
    return this;
}


QueryMaker*
XmlQueryWriter::limitMaxResultSize( int size )
{
    QDomElement e;
    e.setTagName( "limit" );
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
        QDomElement e;
        e.setTagName( "onlyCompilations" );
        m_element.appendChild( e );
    }
    if( mode == OnlyNormalAlbums )
    {
        QDomElement e;
        e.setTagName( "onlyNormalAlbums" );
        m_element.appendChild( e );
    }

    m_qm->setAlbumQueryMode( mode );
    return this;
}

QueryMaker*
XmlQueryWriter::beginAnd()
{
    QDomElement e;
    e.setTagName( "and" );
    m_filterElement.appendChild( e );
    m_filterElement = e;
    m_andorLevel++;
    
    m_qm->beginAnd();
    return this;
}

QueryMaker*
XmlQueryWriter::beginOr()
{
    QDomElement e;
    e.setTagName( "or" );
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

void
XmlQueryWriter::insertRetValue( QString val )
{
    if( m_retvalElement.isNull() )
    {
        m_retvalElement.setTagName( "returnValues" );
        m_element.appendChild( m_retvalElement );
    }

    QDomElement retval;
    retval.setTagName( val );
    m_retvalElement.appendChild( retval );
}


QString
XmlQueryWriter::fieldName( qint64 val )
{
    switch( val )
    {
        case valUrl:         return "url";
        case valTitle:       return "title";
        case valArtist:      return "artist";
        case valAlbum:       return "album";
        case valGenre:       return "genre";
        case valComposer:    return "composer";
        case valYear:        return "year";
        case valComment:     return "comment";
        case valTrackNr:     return "tracknr";
        case valDiscNr:      return "discnr";
        case valLength:      return "length";
        case valBitrate:     return "bitrate";
        case valSamplerate:  return "samplerate";
        case valFilesize:    return "filesize";
        case valFormat:      return "format";
        case valCreateDate:  return "createdate";
        case valScore:       return "score";
        case valRating:      return "rating";
        case valFirstPlayed: return "firstplay";
        case valLastPlayed:  return "listplay";
        case valPlaycount:   return "playcount";
        default:             return "";
    }
}

QString
XmlQueryWriter::compareName( QueryMaker::NumberComparison compare )
{
    if( compare == LessThan )
        return "less";
    else if( compare == GreaterThan )
        return "greater";
    else
        return "equal";
}

