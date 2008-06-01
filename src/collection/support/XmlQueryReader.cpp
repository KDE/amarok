/*
   Copyright (C) 2007-8 Maximilian Kossick <maximilian.kossick@googlemail.com>

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

#include "collection/collectionmanager.h"

struct XmlQueryReader::Private
{
    ReturnValueEnum flag;
    QueryMaker *qm;
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
    : QXmlReader()
    , d( new Private )
{
    d->flag = flag
    d->qm = qm;
}

XmlQueryReader::~XmlQueryReader()
{
    delete d;
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
        {
            if( name() == "and" || name() == "or" )
            {
                d->qm->endAndOr();
                continue;
            }
            else
            {
                break;
            }
        }

        if( isStartElement() )
        {
            if( name() == "filters" )
                readFilters();
            else if( name() == "returnValues" )
                readReturnValues();
            else if( name() == "and" )
                d->qm->beginAnd();
            else if( name() == "or" )
                d->qm->beginOr();
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
                d->qm->startTrackQuery();
            }
            else if( name() == "artists" )
            {
                d->qm->startArtistQuery();
            }
            else if( name() == "albums" )
            {
                d->qm->startAlbumQuery();
            }
            else if( name() == "genres" )
            {
                d->qm->startGenreQuery();
            }
            else if( name() == "composers" )
            {
                d->qm->startComposerQuery() )
            }
            else if( name() == "years" )
            {
                d->qm->startYearQuery();
            }
            else
            {
                if( !customQueryStarted )
                {
                    d->qm->tartCustomQuery();
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
XmlQueryReader::readFilters()
{
    while( !atEnd() )
    {
        readNext();
        if( isEndElement() )
            break;

    }
}

