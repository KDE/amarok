/*
   Copyright (C) 2007-8 Maximilian Kossick <maximilian.kossick@googlemail.com>
   Copyright (C) 2008   Daniel Caleb Jones <danielcjones@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "XmlQueryReader.h"

#include "Debug.h"
#include "collection/CollectionManager.h"

#include <QString>

struct XmlQueryReader::Private
{
    ReturnValueEnum flag;
    QueryMaker *qm;
    QList<Filter> filters;
};

QueryMaker*
XmlQueryReader::getQueryMaker( const QString &xmlData, ReturnValueEnum flag )
{
    QueryMaker *qm = CollectionManager::instance()->queryMaker();
    XmlQueryReader reader( qm, flag );
    if( reader.read( xmlData ) )
        return qm;
    else
        return 0;
}

XmlQueryReader::XmlQueryReader( QueryMaker *qm, ReturnValueEnum flag )
    : QXmlStreamReader()
    , d( new Private )
{
    d->flag = flag;
    d->qm = qm;
}

XmlQueryReader::~XmlQueryReader()
{
    delete d;
}

const QList<XmlQueryReader::Filter>&
XmlQueryReader::getFilters() const
{
    return d->filters;
}

bool
XmlQueryReader::read( const QString &xmlData )
{
    addData( xmlData );
    int queryCount = 0;
    while( !atEnd() )
    {
        readNext();

        if( isStartElement() )
        {
            //we expect exactly one query definition in the xml data.
            //so fail if we find more than one
            if( name() == "query" )
            {
                if( attributes().value( "version" ) == "1.0" )
                {
                    queryCount++;
                    readQuery();
                }
            }
        }
    }

    return queryCount == 1 && !error();
}

void
XmlQueryReader::readQuery()
{
    while( !atEnd() )
    {
        readNext();
        if( isEndElement() )
            break;

        if( isStartElement() )
        {
            if( name() == "filters" )
                readFilters();
            else if( name() == "order" )
            {
                QXmlStreamAttributes attr = attributes();
                QStringRef fieldStr =  attr.value( "field" );
                QStringRef valueStr =  attr.value( "value" );

                if( valueStr == "random" )
                    d->qm->orderByRandom();
                else
                {
                    qint64 field = fieldVal( fieldStr );
                    bool descending = valueStr == "descending";

                    if( field != 0 )
                        d->qm->orderBy( field, descending  );
                }
            }
            else if( name() == "includeCollection" )
            {
                QStringRef id =  attributes().value( "id" );
                if( !name().isEmpty() )
                {
                    d->qm->includeCollection( id.toString() );
                }
            }
            else if( name() == "excludeCollection" )
            {
                QStringRef id =  attributes().value( "id" );
                if( !id.isEmpty() )
                {
                    d->qm->excludeCollection( id.toString() );
                }
            }
            else if( name() == "returnResultAsDataPtrs" )
            {
                d->qm->setReturnResultAsDataPtrs( true );
            }
            else if( name() == "limit" )
            {
                QStringRef value = attributes().value( "value" );
                if( !value.isEmpty() )
                    d->qm->limitMaxResultSize( value.toString().toInt() );
            }
            else if( name() == "onlyCompilations" )
            {
                d->qm->setAlbumQueryMode( QueryMaker::OnlyCompilations );
            }
            else if( name() == "onlyNormalAlbums" )
            {
                d->qm->setAlbumQueryMode( QueryMaker::OnlyNormalAlbums );
            }
            else if( name() == "returnValues" )
                readReturnValues();
            //add more container elements here
            else
                ignoreElements();
        }
    }
}

void
XmlQueryReader::ignoreElements()
{
    //let QXmlStreamReader worry about the fell-formedness of the document
    int depth = 1;
    while( !atEnd() && depth > 0 )
    {
        readNext();
        if( isEndElement() )
            depth--;
        if( isStartElement() )
            depth++;
    }
}

void
XmlQueryReader::readReturnValues()
{
    if( d->flag & XmlQueryReader::IgnoreReturnValues )
    {
        ignoreElements();
        return;
    }
    else
    {
        bool customQueryStarted = false;
        while( !atEnd() )
        {
            readNext();
            if( name() == "tracks" )
            {
                d->qm->setQueryType( QueryMaker::Track );
            }
            else if( name() == "artists" )
            {
                d->qm->setQueryType( QueryMaker::Artist );
            }
            else if( name() == "albums" )
            {
                d->qm->setQueryType( QueryMaker::Album );
            }
            else if( name() == "genres" )
            {
                d->qm->setQueryType( QueryMaker::Genre );
            }
            else if( name() == "composers" )
            {
                d->qm->setQueryType( QueryMaker::Composer );
            }
            else if( name() == "year" )
            {
                d->qm->setQueryType( QueryMaker::Year );
            }
            else
            {
                if( !customQueryStarted )
                {
                    d->qm->setQueryType( QueryMaker::Custom );
                }
                //TODO write a mapping function somewhere
                if( name() == "title" )
                {
                    d->qm->addReturnValue( QueryMaker::valTitle );
                }
                else if( name() == "artist" )
                {
                    d->qm->addReturnValue( QueryMaker::valArtist );
                }
            }
        }
    }
}

void
XmlQueryReader::readAndOr()
{
    readFilters();
    ignoreElements();
    d->qm->endAndOr();
}


void
XmlQueryReader::readFilters()
{
    while( !atEnd() )
    {
        readNext();
        if( isEndElement() ) 
            break;
        
        if( name() == "include" || name() == "exclude" )
        {
            Filter filter;

            QXmlStreamAttributes attr = attributes();

            filter.value = attr.value( "value" ).toString();
            filter.field = fieldVal( attr.value( "field" ) );

            if( filter.field == 0 )
                break;

            filter.compare = compareVal( attr.value( "compare" ) );

            if( filter.compare != -1 )
            {
                qint64 numValue = filter.value.toInt();
                if( name() == "include" )
                {
                    debug() << "XQR: number include filter:";
                    d->qm->addNumberFilter( filter.field, numValue,
                            (QueryMaker::NumberComparison)filter.compare );
                }
                else
                {
                    debug() << "XQR: number exclude filter: ";
                    d->qm->excludeNumberFilter( filter.field, numValue,
                            (QueryMaker::NumberComparison)filter.compare );
                }
            }
            else
            {
                if( name() == "include" )
                {
                    debug() << "QXR: include filter";
                    d->qm->addFilter( filter.field, filter.value );
                }
                else
                {
                    debug() << "QXR: exclude filter";
                    d->qm->excludeFilter( filter.field, filter.value );
                }
            }

            d->filters.append( filter );
        }
        else if( name() == "and" )
        {
            d->qm->beginAnd();
            readAndOr();
        }
        else if( name() == "or" )
        {
            d->qm->beginOr();
            readAndOr();
        }
    }
}

qint64
XmlQueryReader::fieldVal( QStringRef field )
{
    if     ( field == "url"        ) return QueryMaker::valUrl;
    else if( field == "title"      ) return QueryMaker::valTitle;
    else if( field == "artist"     ) return QueryMaker::valArtist;
    else if( field == "album"      ) return QueryMaker::valAlbum;
    else if( field == "genre"      ) return QueryMaker::valGenre;
    else if( field == "composer"   ) return QueryMaker::valComposer;
    else if( field == "year"       ) return QueryMaker::valYear;
    else if( field == "comment"    ) return QueryMaker::valComment; 
    else if( field == "tracknr"    ) return QueryMaker::valTrackNr;
    else if( field == "discnr"     ) return QueryMaker::valDiscNr;
    else if( field == "length"     ) return QueryMaker::valLength;
    else if( field == "bitrate"    ) return QueryMaker::valBitrate;
    else if( field == "samplerate" ) return QueryMaker::valSamplerate;
    else if( field == "filesize"   ) return QueryMaker::valFilesize;
    else if( field == "format"     ) return QueryMaker::valFormat;
    else if( field == "createdate" ) return QueryMaker::valCreateDate;
    else if( field == "score"      ) return QueryMaker::valScore;
    else if( field == "rating"     ) return QueryMaker::valRating;
    else if( field == "firstplay"  ) return QueryMaker::valFirstPlayed;
    else if( field == "lastplay"   ) return QueryMaker::valLastPlayed;
    else if( field == "playcount"  ) return QueryMaker::valPlaycount;
    else                             return 0;
}

int
XmlQueryReader::compareVal( QStringRef compare )
{
    if( compare == "less" )
        return QueryMaker::LessThan;
    else if( compare == "greater" )
        return QueryMaker::GreaterThan;
    else if( compare == "equals" )
        return QueryMaker::Equals;
    else
        return -1;
}

