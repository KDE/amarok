/****************************************************************************************
 * Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
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

#include "XmlQueryReader.h"

#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <QString>

struct XmlQueryReader::Private
{
    ReturnValueEnum flag;
    Collections::QueryMaker *qm;
    QList<Filter> filters;
};

Collections::QueryMaker*
XmlQueryReader::getQueryMaker( const QString &xmlData, ReturnValueEnum flag )
{
    Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
    XmlQueryReader reader( qm, flag );
    if( reader.read( xmlData ) )
        return qm;
    else
        return nullptr;
}

XmlQueryReader::XmlQueryReader( Collections::QueryMaker *qm, ReturnValueEnum flag )
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
                if( attributes().value( QStringLiteral("version") ) == "1.0" )
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

        if( isStartElement() )
        {
            if( name() == "filters" )
                readFilters();
            else if( name() == "order" )
            {
                QXmlStreamAttributes attr = attributes();
                QStringView fieldStr =  attr.value( QStringLiteral("field") );
                QStringView valueStr =  attr.value( QStringLiteral("value") );

                qint64 field = Meta::fieldForName( fieldStr.toString() );
                bool descending = valueStr == QStringLiteral("descending");

                if( field != 0 )
                    d->qm->orderBy( field, descending  );
            }
            else if( name() == "limit" )
            {
                QStringView value = attributes().value( QStringLiteral("value") );
                if( !value.isEmpty() )
                    d->qm->limitMaxResultSize( value.toString().toInt() );
            }
            else if( name() == "onlyCompilations" )
            {
                d->qm->setAlbumQueryMode( Collections::QueryMaker::OnlyCompilations );
            }
            else if( name() == "onlyNormalAlbums" )
            {
                d->qm->setAlbumQueryMode( Collections::QueryMaker::OnlyNormalAlbums );
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
                d->qm->setQueryType( Collections::QueryMaker::Track );
            }
            else if( name() == "artists" )
            {
                d->qm->setQueryType( Collections::QueryMaker::Artist );
            }
            else if( name() == "albums" )
            {
                d->qm->setQueryType( Collections::QueryMaker::Album );
            }
            else if( name() == "albumartist" )
            {
                d->qm->setQueryType( Collections::QueryMaker::AlbumArtist );
            }
            else if( name() == "genres" )
            {
                d->qm->setQueryType( Collections::QueryMaker::Genre );
            }
            else if( name() == "composers" )
            {
                d->qm->setQueryType( Collections::QueryMaker::Composer );
            }
            else if( name() == "year" )
            {
                d->qm->setQueryType( Collections::QueryMaker::Year );
            }
            else
            {
                if( !customQueryStarted )
                {
                    d->qm->setQueryType( Collections::QueryMaker::Custom );
                }
                //TODO write a mapping function somewhere
                if( name() == "title" )
                {
                    d->qm->addReturnValue( Meta::valTitle );
                }
                else if( name() == "artist" )
                {
                    d->qm->addReturnValue( Meta::valArtist );
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

XmlQueryReader::Filter
XmlQueryReader::readFilter(QXmlStreamReader *reader)
{
    Filter filter;

    QXmlStreamAttributes attr = reader->attributes();

    filter.exclude = (reader->name() != "include");
    filter.field = Meta::fieldForName( attr.value( QStringLiteral("field") ).toString() );
    filter.value = attr.value( QStringLiteral("value") ).toString();

    QStringView compareStr = attr.value( QStringLiteral("compare") );
    if( compareStr.isEmpty() )
        filter.compare = -1;
    else
        filter.compare = compareVal( compareStr );

    return filter;
}

void
XmlQueryReader::readFilters()
{
    while( !atEnd() )
    {
        readNext();
        if( isEndElement() )
        {
            if( name() == QStringLiteral("and") || name() == QStringLiteral("or") )
            {
                d->qm->endAndOr();
                break;
            }
            else if( name() == "filters" )
            {
                break;
            }
            else
                continue;
        }

        if( name() == QStringLiteral("include") || name() == QStringLiteral("exclude") )
        {
            Filter filter = readFilter(this);

            if( filter.field == 0 )
                break;

            if( filter.compare != -1 )
            {
                qint64 numValue = filter.value.toInt();
                if( !filter.exclude )
                {
                    debug() << "XQR: number include filter:";
                    d->qm->addNumberFilter( filter.field, numValue,
                            (Collections::QueryMaker::NumberComparison)filter.compare );
                }
                else
                {
                    debug() << "XQR: number exclude filter: ";
                    d->qm->excludeNumberFilter( filter.field, numValue,
                            (Collections::QueryMaker::NumberComparison)filter.compare );
                }
            }
            else
            {
                if( !filter.exclude )
                {
                    debug() << "XQR: include filter";
                    d->qm->addFilter( filter.field, filter.value );
                }
                else
                {
                    debug() << "XQR: exclude filter";
                    d->qm->excludeFilter( filter.field, filter.value );
                }
            }

            d->filters.append( filter );
        }
        else if( name() == QStringLiteral("and") )
        {
            d->qm->beginAnd();
            readFilters();
        }
        else if( name() == QStringLiteral("or") )
        {
            d->qm->beginOr();
            readFilters();
        }
    }
}

int
XmlQueryReader::compareVal( QStringView compare )
{
    if( compare == QStringLiteral("less") )
        return Collections::QueryMaker::LessThan;
    else if( compare == QStringLiteral("greater") )
        return Collections::QueryMaker::GreaterThan;
    else if( compare == QStringLiteral("equals") )
        return Collections::QueryMaker::Equals;
    else
        return -1;
}
