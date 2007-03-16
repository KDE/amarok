/* This file is part of the KDE project
    Copyright (C) 2004 Mark Kretschmann <markey@web.de>
    Copyright (C) 2004 Christian Muehlhaeuser <chris@chris.de>
    Copyright (C) 2004 Sami Nieminen <sami.nieminen@iki.fi>
    Copyright (C) 2005 Ian Monroe <ian@monroe.nu>
    Copyright (C) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com>
    Copyright (C) 2005 Isaiah Damron <xepo@trifault.net>
    Copyright (C) 2005-2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>
    Copyright (C) 2006 Jonas Hurrelmann <j@outpo.st>
    Copyright (C) 2006 Shane King <kde@dontletsstart.com>
    Copyright (C) 2006 Peter C. Ndikuwera <pndiku@gmail.com>

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

#include "querybuilder.h"

#include "amarokconfig.h"
#include "collectiondb.h"
#include "expression.h"
#include "mountpointmanager.h"

#include <klocale.h>

//////////////////////////////////////////////////////////////////////////////////////////
// CLASS QueryBuilder
//////////////////////////////////////////////////////////////////////////////////////////

QueryBuilder::QueryBuilder()
{
    m_OR.push(false);
    clear();
    // there are a few string members with a large number of appends. to
    // avoid reallocations, pre-reserve 1024 bytes and try never to assign
    // it, instead doing setLength(0) and appends
    // Note: unfortunately, QT3's setLength(), which is also called from append,
    // squeezes the string if it's less than 4x the length. So this is useless.
    // Uncomment after porting to QT4 if it's smarter about this, as the docs say.
//     m_query.reserve(1024);
//     m_values.reserve(1024);
//     m_tables.reserve(1024);
}


void
QueryBuilder::linkTables( int tables )
{
    m_tables.setLength(0);

    m_tables += tableName( tabSong );

    if ( !(tables & tabSong ) )
    {
        // check if only one table is selected (does somebody know a better way to check that?)
        if (tables == tabAlbum || tables==tabArtist || tables==tabGenre || tables == tabYear || tables == tabStats || tables == tabPodcastEpisodes || tables == tabPodcastFolders || tables == tabPodcastChannels || tables == tabLabels) {
        m_tables.setLength( 0 );
        m_tables += tableName(tables);
    }
        else
            tables |= tabSong;
    }

    if ( tables & tabSong )
    {
        if ( tables & tabAlbum )
            ((m_tables += " LEFT JOIN ") += tableName( tabAlbum)) += " ON album.id=tags.album";
        if ( tables & tabArtist )
            ((m_tables += " LEFT JOIN ") += tableName( tabArtist)) += " ON artist.id=tags.artist";
        if ( tables & tabComposer )
            ((m_tables += " LEFT JOIN ") += tableName( tabComposer)) += " ON composer.id=tags.composer";
        if ( tables & tabGenre )
            ((m_tables += " LEFT JOIN ") += tableName( tabGenre)) += " ON genre.id=tags.genre";
        if ( tables & tabYear )
            ((m_tables += " LEFT JOIN ") += tableName( tabYear)) += " ON year.id=tags.year";
        if ( tables & tabStats )
        {
            ((m_tables += " LEFT JOIN ") += tableName( tabStats))
                                      += " ON statistics.url=tags.url AND statistics.deviceid = tags.deviceid";
            //if ( !m_url.isEmpty() ) {
            //    QString url = QString( '.' ) + m_url;
            //    m_tables += QString( " OR statistics.deviceid = -1 AND statistics.url = '%1'" )
            //                                    .arg( CollectionDB::instance()->escapeString( url ) );
            //}
        }
        if ( tables & tabLyrics )
            ((m_tables += " LEFT JOIN ") += tableName( tabLyrics))
                                      += " ON lyrics.url=tags.url AND lyrics.deviceid = tags.deviceid";

        if ( tables & tabDevices )
            ((m_tables += " LEFT JOIN ") += tableName( tabDevices )) += " ON tags.deviceid = devices.id";
        if ( tables & tabLabels )
            ( m_tables += " LEFT JOIN tags_labels ON tags.url = tags_labels.url AND tags.deviceid = tags_labels.deviceid" )
                += " LEFT JOIN labels ON tags_labels.labelid = labels.id";
    }
}


void
QueryBuilder::addReturnValue( int table, qint64 value, bool caseSensitive /* = false, unless value refers to a string */ )
{
    caseSensitive |= value == valName || value == valTitle || value == valComment;

    if ( !m_values.isEmpty() && m_values != "DISTINCT " ) m_values += ',';

    if ( value == valDummy )
        m_values += "''";
    else
    {
        if ( caseSensitive && CollectionDB::instance()->getType() == DbConnection::mysql )
            m_values += "BINARY ";
        m_values += tableName( table ) + '.';
        m_values += valueName( value );
    }

    m_linkTables |= table;
    m_returnValues++;
    if ( value & valURL )
    {
        // make handling of deviceid transparent to calling code
        m_deviceidPos = m_returnValues + 1;  //the return value after the url is the deviceid
        m_values += ',';
        m_values += tableName( table );
        m_values += '.';
        m_values += valueName( valDeviceId );
    }
}

void
QueryBuilder::addReturnFunctionValue( int function, int table, qint64 value)
{
    // translate NULL and 0 values into the default value for percentage/rating
    // First translate 0 to NULL via NULLIF, then NULL to default via COALESCE
    bool defaults = function == funcAvg && ( value & valScore || value & valRating );

    if ( !m_values.isEmpty() && m_values != "DISTINCT " ) m_values += ',';
    m_values += functionName( function ) + '(';
    if ( defaults )
        m_values += "COALESCE(NULLIF(";
    m_values += tableName( table ) + '.';
    m_values += valueName( value );
    if ( defaults )
    {
        m_values += ", 0), ";
        if ( value & valScore )
            m_values += "50";
        else
            m_values += '6';
        m_values += ')';
    }
    m_values += ") AS ";
    m_values += functionName( function )+tableName( table )+valueName( value );

    m_linkTables |= table;
    if ( !m_showAll ) m_linkTables |= tabSong;
    m_returnValues++;
}

uint
QueryBuilder::countReturnValues()
{
    return m_returnValues;
}

void
QueryBuilder::addUrlFilters( const QStringList& filter )
{
    if ( !filter.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + ' ';

        for ( uint i = 0; i < filter.count(); i++ )
        {
                int deviceid = MountPointManager::instance()->getIdForUrl( filter[i] );
                QString rpath = MountPointManager::instance()->getRelativePath( deviceid , filter[i] );
                m_where += "OR (tags.url = '" + CollectionDB::instance()->escapeString( rpath ) + "' ";
                m_where += QString( "AND tags.deviceid = %1 ) " ).arg( QString::number( deviceid ) );
                //TODO MountPointManager fix this
        }

        m_where += " ) ";
    }

    m_linkTables |= tabSong;
}

void
QueryBuilder::setGoogleFilter( int defaultTables, QString query )
{
    //TODO MountPointManager fix google syntax
    //no clue about what needs to be done atm
    ParsedExpression parsed = ExpressionParser::parse( query );

    for( uint i = 0, n = parsed.count(); i < n; ++i ) //check each part for matchiness
    {
        beginOR();
        for( uint ii = 0, nn = parsed[i].count(); ii < nn; ++ii )
        {
            const expression_element &e = parsed[i][ii];
            QString s = e.text;
            int mode;
            switch( e.match )
            {
                case expression_element::More:     mode = modeGreater; break;
                case expression_element::Less:     mode = modeLess;    break;
                case expression_element::Contains:
                default:                           mode = modeNormal;  break;
            }
            bool exact = false; // enable for numeric values

            int table = -1;
            qint64 value = -1;
            if( e.field == "artist" )
                table = tabArtist;
            else if( e.field == "composer" )
                table = tabComposer;
            else if( e.field == "album" )
                table = tabAlbum;
            else if( e.field == "title" )
                table = tabSong;
            else if( e.field == "genre" )
                table = tabGenre;
            else if( e.field == "year" )
            {
                table = tabYear;
                value = valName;
                exact = true;
            }
            else if( e.field == "score" )
            {
                table = tabStats;
                value = valScore;
                exact = true;
            }
            else if( e.field == "rating" )
            {
                table = tabStats;
                value = valRating;
                exact = true;
                s = QString::number( int( s.toFloat() * 2 ) );
            }
            else if( e.field == "directory" )
            {
                table = tabSong;
                value = valDirectory;
            }
            else if( e.field == "length" )
            {
                table = tabSong;
                value = valLength;
                exact = true;
            }
            else if( e.field == "playcount" )
            {
                table = tabStats;
                value = valPlayCounter;
                exact = true;
            }
            else if( e.field == "samplerate" )
            {
                table = tabSong;
                value = valSamplerate;
                exact = true;
            }
            else if( e.field == "track" )
            {
                table = tabSong;
                value = valTrack;
                exact = true;
            }
            else if( e.field == "disc" || e.field == "discnumber" )
            {
                table = tabSong;
                value = valDiscNumber;
                exact = true;
            }
            else if( e.field == "size" || e.field == "filesize" )
            {
                table = tabSong;
                value = valFilesize;
                exact = true;
                if( s.toLower().endsWith( "m" ) )
                    s = QString::number( s.left( s.length()-1 ).toLong() * 1024 * 1024 );
                else if( s.toLower().endsWith( "k" ) )
                    s = QString::number( s.left( s.length()-1 ).toLong() * 1024 );
            }
            else if( e.field == "filename" || e.field == "url" )
            {
                table = tabSong;
                value = valURL;
            }
            else if( e.field == "filetype" || e.field == "type" )
            {
                table = tabSong;
                value = valURL;
                mode = modeEndMatch;
                s.prepend( '.' );
            }
            else if( e.field == "bitrate" )
            {
                table = tabSong;
                value = valBitrate;
                exact = true;
            }
            else if( e.field == "comment" )
            {
                table = tabSong;
                value = valComment;
            }
            else if( e.field == "bpm" )
            {
                table = tabSong;
                value = valBPM;
                exact = true;
            }
            else if( e.field == "lyrics" )
            {
                table = tabLyrics;
                value = valLyrics;
            }
            else if( e.field == "device" )
            {
                table = tabDevices;
                value = valDeviceLabel;
            }
            else if( e.field == "mountpoint" )
            {
                table = tabDevices;
                value = valMountPoint;
            }
            else if( e.field == "label" )
            {
                table = tabLabels;
                value = valName;
            }

            if( e.negate )
            {
                if( value >= 0 )
                    excludeFilter( table, value, s, mode, exact );
                else
                    excludeFilter( table >= 0 ? table : defaultTables, s );
            }
            else
            {
                if( value >= 0 )
                    addFilter( table, value, s, mode, exact );
                else
                    addFilter( table >= 0 ? table : defaultTables, s );
            }
        }
        endOR();
    }
}

void
QueryBuilder::addFilter( int tables, const QString& filter )
{
    if ( !filter.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + ' ';

        if ( tables & tabAlbum )
            m_where += "OR album.name " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabArtist )
            m_where += "OR artist.name " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabComposer )
            m_where += "OR composer.name " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabGenre )
            m_where += "OR genre.name " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabYear )
            m_where += "OR year.name " + CollectionDB::likeCondition( filter, false, false );
        if ( tables & tabSong )
            m_where += "OR tags.title " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabLabels )
            m_where += "OR labels.name " + CollectionDB::likeCondition( filter, true, true );

        if ( i18n( "Unknown" ).contains( filter, false ) )
        {
            if ( tables & tabAlbum )
                m_where += "OR album.name = '' ";
            if ( tables & tabArtist )
                m_where += "OR artist.name = '' ";
            if ( tables & tabComposer )
                m_where += "OR composer.name = '' ";
            if ( tables & tabGenre )
                m_where += "OR genre.name = '' ";
            if ( tables & tabYear )
                m_where += "OR year.name = '' ";
            if ( tables & tabSong )
                m_where += "OR tags.title = '' ";
        }
        if ( ( tables & tabArtist ) && i18n( "Various Artists" ).contains( filter, false ) )
            m_where += QString( "OR tags.sampler = %1 " ).arg( CollectionDB::instance()->boolT() );
        m_where += " ) ";
    }

    m_linkTables |= tables;
}

void
QueryBuilder::addFilter( int tables, qint64 value, const QString& filter, int mode, bool exact )
{
    //true for INTEGER fields (see comment of coalesceField(int, qint64)
    bool useCoalesce = coalesceField( tables, value );
    m_where += ANDslashOR() + " ( ";

    QString m, s;
    if (mode == modeLess || mode == modeGreater)
    {
        QString escapedFilter;
        if (useCoalesce && DbConnection::sqlite == CollectionDB::instance()->getDbConnectionType())
            escapedFilter = CollectionDB::instance()->escapeString( filter );
        else
            escapedFilter = '\'' + CollectionDB::instance()->escapeString( filter ) + "' ";
        s = ( mode == modeLess ? "< " : "> " ) + escapedFilter;
    }
    else
    {
        if (exact)
            if (useCoalesce && DbConnection::sqlite == CollectionDB::instance()->getDbConnectionType())
                s = " = " +CollectionDB::instance()->escapeString( filter ) + ' ';
            else
                s = " = '" + CollectionDB::instance()->escapeString( filter ) + "' ";
        else
            s = CollectionDB::likeCondition( filter, mode != modeBeginMatch, mode != modeEndMatch );
    }

    if( coalesceField( tables, value ) )
        m_where += QString( "COALESCE(%1.%2,0) " ).arg( tableName( tables ) ).arg( valueName( value ) ) + s;
    else
        m_where += QString( "%1.%2 " ).arg( tableName( tables ) ).arg( valueName( value ) ) + s;

    if ( !exact && (value & valName) && mode == modeNormal && i18n( "Unknown").contains( filter, false ) )
        m_where += QString( "OR %1.%2 = '' " ).arg( tableName( tables ) ).arg( valueName( value ) );

    m_where += " ) ";

    m_linkTables |= tables;
}

void
QueryBuilder::addFilters( int tables, const QStringList& filter )
{
    if ( !filter.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolT() + ' ';

        for ( uint i = 0; i < filter.count(); i++ )
        {
            m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + ' ';

            if ( tables & tabAlbum )
                m_where += "OR album.name " + CollectionDB::likeCondition( filter[i], true, true );
            if ( tables & tabArtist )
                m_where += "OR artist.name " + CollectionDB::likeCondition( filter[i], true, true );
            if ( tables & tabComposer )
                m_where += "OR composer.name " + CollectionDB::likeCondition( filter[i], true, true );
            if ( tables & tabGenre )
                m_where += "OR genre.name " + CollectionDB::likeCondition( filter[i], true, true );
            if ( tables & tabYear )
                m_where += "OR year.name " + CollectionDB::likeCondition( filter[i], false, false );
            if ( tables & tabSong )
                m_where += "OR tags.title " + CollectionDB::likeCondition( filter[i], true, true );
            if ( tables & tabLabels )
                m_where += "OR labels.name " + CollectionDB::likeCondition( filter[i], true, true );

            if ( i18n( "Unknown" ).contains( filter[i], false ) )
            {
                if ( tables & tabAlbum )
                    m_where += "OR album.name = '' ";
                if ( tables & tabArtist )
                    m_where += "OR artist.name = '' ";
                if ( tables & tabComposer )
                    m_where += "OR composer.name = '' ";
                if ( tables & tabGenre )
                    m_where += "OR genre.name = '' ";
                if ( tables & tabYear )
                    m_where += "OR year.name = '' ";
                if ( tables & tabSong )
                    m_where += "OR tags.title = '' ";
            }
            if ( i18n( "Various Artists" ).contains( filter[ i ], false ) && ( tables & tabArtist ) )
                m_where += "OR tags.sampler = " + CollectionDB::instance()->boolT() + ' ';
            m_where += " ) ";
        }

        m_where += " ) ";
    }

    m_linkTables |= tables;
}

void
QueryBuilder::excludeFilter( int tables, const QString& filter )
{
    if ( !filter.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolT() + ' ';


        if ( tables & tabAlbum )
            m_where += "AND album.name NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabArtist )
            m_where += "AND artist.name NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabComposer )
            m_where += "AND composer.name NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabGenre )
            m_where += "AND genre.name NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabYear )
            m_where += "AND year.name NOT " + CollectionDB::likeCondition( filter, false, false );
        if ( tables & tabSong )
            m_where += "AND tags.title NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabLabels )
            m_where += "AND labels.name NOT " + CollectionDB::likeCondition( filter, true, true );

        if ( i18n( "Unknown" ).contains( filter, false ) )
        {
            if ( tables & tabAlbum )
                m_where += "AND album.name <> '' ";
            if ( tables & tabArtist )
                m_where += "AND artist.name <> '' ";
            if ( tables & tabComposer )
                m_where += "AND composer.name <> '' ";
            if ( tables & tabGenre )
                m_where += "AND genre.name <> '' ";
            if ( tables & tabYear )
                m_where += "AND year.name <> '' ";
            if ( tables & tabSong )
                m_where += "AND tags.title <> '' ";
        }

       if ( i18n( "Various Artists" ).contains( filter, false ) && (  tables & tabArtist ) )
            m_where += "AND tags.sampler = " + CollectionDB::instance()->boolF() + ' ';


        m_where += " ) ";
    }

    m_linkTables |= tables;
}

void
QueryBuilder::excludeFilter( int tables, qint64 value, const QString& filter, int mode, bool exact )
{
    m_where += ANDslashOR() + " ( ";

    QString m, s;
    if (mode == modeLess || mode == modeGreater)
        s = ( mode == modeLess ? ">= '" : "<= '" ) + CollectionDB::instance()->escapeString( filter ) + "' ";
    else
    {
        if (exact)
        {
            bool isNumber;
            filter.toInt( &isNumber );
            if (isNumber)
                s = " <> " + CollectionDB::instance()->escapeString( filter ) + ' ';
            else
                s = " <> '" + CollectionDB::instance()->escapeString( filter ) + "' ";
        }
        else
            s = "NOT " + CollectionDB::instance()->likeCondition( filter, mode != modeBeginMatch, mode != modeEndMatch ) + ' ';
    }

    if( coalesceField( tables, value ) )
        m_where += QString( "COALESCE(%1.%2,0) " ).arg( tableName( tables ) ).arg( valueName( value ) ) + s;
    else
        m_where += QString( "%1.%2 " ).arg( tableName( tables ) ).arg( valueName( value ) ) + s;

    if ( !exact && (value & valName) && mode == modeNormal && i18n( "Unknown").contains( filter, false ) )
        m_where += QString( "AND %1.%2 <> '' " ).arg( tableName( tables ) ).arg( valueName( value ) );

    m_where += " ) ";

    m_linkTables |= tables;
}

void
QueryBuilder::addMatch( int tables, const QString& match, bool interpretUnknown /* = true */, bool caseSensitive /* = true */ )
{
    QString matchCondition = caseSensitive ? CollectionDB::exactCondition( match ) : CollectionDB::likeCondition( match );

    (((m_where += ANDslashOR()) += " ( ") += CollectionDB::instance()->boolF()) += ' ';
    if ( tables & tabAlbum )
        (m_where += "OR album.name ") += matchCondition;
    if ( tables & tabArtist )
        (m_where += "OR artist.name ") += matchCondition;
    if ( tables & tabComposer )
        (m_where += "OR composer.name ") += matchCondition;
    if ( tables & tabGenre )
        (m_where += "OR genre.name ") += matchCondition;
    if ( tables & tabYear )
        (m_where += "OR year.name ") += matchCondition;
    if ( tables & tabSong )
        (m_where += "OR tags.title ") += matchCondition;
    if ( tables & tabLabels )
        (m_where += "OR labels.name ") += matchCondition;

    static QString i18nUnknown = i18n("Unknown");

    if ( interpretUnknown && match == i18nUnknown )
    {
        if ( tables & tabAlbum ) m_where += "OR album.name = '' ";
        if ( tables & tabArtist ) m_where += "OR artist.name = '' ";
        if ( tables & tabComposer ) m_where += "OR composer.name = '' ";
        if ( tables & tabGenre ) m_where += "OR genre.name = '' ";
        if ( tables & tabYear ) m_where += "OR year.name = '' ";
    }
    if ( tables & tabLabels && match.isEmpty() )
        m_where += " OR labels.name IS NULL ";
    m_where += " ) ";

    m_linkTables |= tables;
}


void
QueryBuilder::addMatch( int tables, qint64 value, const QString& match, bool interpretUnknown /* = true */, bool caseSensitive /* = true */  )
{
    m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + ' ';
    if ( value & valURL )
        m_url = match;
    //FIXME max: doesn't work yet if we are querying for the mount point part of a directory
    if ( value & valURL || value & valDirectory )
    {
        int deviceid = MountPointManager::instance()->getIdForUrl( match );
        QString rpath = MountPointManager::instance()->getRelativePath( deviceid, match );
        //we are querying for a specific path, so we don't need the tags.deviceid IN (...) stuff
        //which is automatially appended if m_showAll = false
        m_showAll = true;
        m_where += QString( "OR %1.%2 " )
            .arg( tableName( tables ) )
            .arg( valueName( value ) );
        m_where += caseSensitive ? CollectionDB::exactCondition( rpath ) : CollectionDB::likeCondition( rpath );
        m_where += QString( " AND %1.deviceid = %2 " ).arg( tableName( tables ) ).arg( deviceid );
        if ( deviceid != -1 )
        {
            //handle corner case
            QString rpath2( '.' + match );
            m_where += QString( " OR %1.%2 " ).arg( tableName( tables ) ).arg( valueName( value ) );
            m_where += caseSensitive ? CollectionDB::exactCondition( rpath2 ) : CollectionDB::likeCondition( rpath2 );
            m_where += QString( " AND %1.deviceid = -1 " ).arg( tableName( tables ) );
        }
    }
    else
    {
        m_where += QString( "OR %1.%2 " ).arg( tableName( tables ) ).arg( valueName( value ) );
        m_where += caseSensitive ? CollectionDB::exactCondition( match ) : CollectionDB::likeCondition( match );
    }

    if ( ( value & valName ) && interpretUnknown && match == i18n( "Unknown" ) )
        m_where += QString( "OR %1.%2 = '' " ).arg( tableName( tables ) ).arg( valueName( value ) );

    m_where += " ) ";

    m_linkTables |= tables;
}


void
QueryBuilder::addMatches( int tables, const QStringList& match, bool interpretUnknown /* = true */, bool caseSensitive /* = true */ )
{
    QStringList matchConditions;
    for ( uint i = 0; i < match.count(); i++ )
        matchConditions << ( caseSensitive ? CollectionDB::exactCondition( match[i] ) : CollectionDB::likeCondition( match[i] ) );

    m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + ' ';

    for ( uint i = 0; i < match.count(); i++ )
    {
        if ( tables & tabAlbum )
            m_where += "OR album.name " + matchConditions[ i ];
        if ( tables & tabArtist )
            m_where += "OR artist.name " + matchConditions[ i ];
        if ( tables & tabComposer )
            m_where += "OR composer.name " + matchConditions[ i ];
        if ( tables & tabGenre )
            m_where += "OR genre.name " + matchConditions[ i ];
        if ( tables & tabYear )
            m_where += "OR year.name " + matchConditions[ i ];
        if ( tables & tabSong )
            m_where += "OR tags.title " + matchConditions[ i ];
        if ( tables & tabStats )
            m_where += "OR statistics.url " + matchConditions[ i ];
        if ( tables & tabLabels )
            (m_where += "OR labels.name ") += matchConditions[ i ];


        if ( interpretUnknown && match[i] == i18n( "Unknown" ) )
        {
            if ( tables & tabAlbum ) m_where += "OR album.name = '' ";
            if ( tables & tabArtist ) m_where += "OR artist.name = '' ";
            if ( tables & tabComposer ) m_where += "OR composer.name = '' ";
            if ( tables & tabGenre ) m_where += "OR genre.name = '' ";
            if ( tables & tabYear ) m_where += "OR year.name = '' ";
        }
        if ( tables & tabLabels && match[i].isEmpty() )
            m_where += " OR labels.name IS NULL ";
    }

    m_where += " ) ";
    m_linkTables |= tables;
}

void
QueryBuilder::excludeMatch( int tables, const QString& match )
{
    m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolT() + ' ';
    if ( tables & tabAlbum ) m_where += "AND album.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
    if ( tables & tabArtist ) m_where += "AND artist.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
    if ( tables & tabComposer ) m_where += "AND composer.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
    if ( tables & tabGenre ) m_where += "AND genre.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
    if ( tables & tabYear ) m_where += "AND year.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
    if ( tables & tabSong ) m_where += "AND tags.title <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
    if ( tables & tabLabels ) m_where += "AND labels.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";

    if ( match == i18n( "Unknown" ) )
    {
        if ( tables & tabAlbum ) m_where += "AND album.name <> '' ";
        if ( tables & tabArtist ) m_where += "AND artist.name <> '' ";
        if ( tables & tabComposer ) m_where += "AND composer.name <> '' ";
        if ( tables & tabGenre ) m_where += "AND genre.name <> '' ";
        if ( tables & tabYear ) m_where += "AND year.name <> '' ";
    }
    m_where += " ) ";

    m_linkTables |= tables;
}


void
QueryBuilder::exclusiveFilter( int tableMatching, int tableNotMatching, qint64 value )
{
    m_where += " AND ";
    m_where += tableName( tableNotMatching ) + '.';
    m_where += valueName( value );
    m_where += " IS null ";

    m_linkTables |= tableMatching;
    m_linkTables |= tableNotMatching;
}


void
QueryBuilder::addNumericFilter(int tables, qint64 value, const QString &n,
                               int mode /* = modeNormal */,
                               const QString &endRange /* = QString::null */ )
{
    m_where.append( ANDslashOR() ).append( " ( " );

    if ( coalesceField( tables, value) )
        m_where.append("COALESCE(");

    m_where.append( tableName( tables ) ).append( '.' ).append( valueName( value ) );

    if ( coalesceField( tables, value) )
        m_where.append(",0)");

    switch (mode) {
    case modeNormal:
        m_where.append( " = " ); break;
    case modeLess:
        m_where.append( " < " ); break;
    case modeGreater:
        m_where.append( " > " ); break;
    case modeBetween:
        m_where.append( " BETWEEN " ); break;
    case modeNotBetween:
        m_where.append(" NOT BETWEEN "); break;
    default:
        qWarning( "Unhandled mode in addNumericFilter, using equals: %d", mode );
        m_where.append( " = " );
    }

    m_where.append( n );
    if ( mode == modeBetween || mode == modeNotBetween )
        m_where.append( " AND " ).append( endRange );

    m_where.append( " ) " );
    m_linkTables |= tables;
}



void
QueryBuilder::setOptions( int options )
{
    if ( options & optNoCompilations || options & optOnlyCompilations )
        m_linkTables |= tabSong;

    if ( options & optNoCompilations ) m_where += QString("AND tags.sampler = %1 ").arg(CollectionDB::instance()->boolF());
    if ( options & optOnlyCompilations ) m_where += QString("AND tags.sampler = %1 ").arg(CollectionDB::instance()->boolT());

    if (CollectionDB::instance()->getType() == DbConnection::postgresql && options & optRemoveDuplicates && options & optRandomize)
    {
            m_values = "DISTINCT " + CollectionDB::instance()->randomFunc() + " AS __random "+ m_values;
            if ( !m_sort.isEmpty() )
                m_sort += ',';
            m_sort += CollectionDB::instance()->randomFunc() + ' ';
    }
    else
    {
            if ( options & optRemoveDuplicates )
                m_values = "DISTINCT " + m_values;
            if ( options & optRandomize )
            {
                if ( !m_sort.isEmpty() ) m_sort += ',';
                m_sort += CollectionDB::instance()->randomFunc() + ' ';
            }
    }

    if ( options & optShowAll ) m_showAll = true;
}


void
QueryBuilder::sortBy( int table, qint64 value, bool descending )
{
    //shall we sort case-sensitively? (not for integer columns!)
    bool b = true;
    if ( value & valID || value & valTrack || value & valScore || value & valRating || value & valLength || value & valBitrate ||
         value & valSamplerate || value & valPlayCounter || value & valAccessDate || value & valCreateDate ||
         value & valFilesize || value & valDiscNumber ||
         table & tabYear )
        b = false;

	// only coalesce for certain columns
	bool c = false;
    if ( value & valScore || value & valRating || value & valPlayCounter || value & valAccessDate || value & valCreateDate )
		c = true;

    if ( !m_sort.isEmpty() ) m_sort += ',';
    if ( b ) m_sort += "LOWER( ";
    if ( c ) m_sort += "COALESCE( ";

    m_sort += tableName( table ) + '.';
    m_sort += valueName( value );

    if ( c ) m_sort += ", 0 )";

    if ( b ) m_sort += " ) ";
    if ( descending ) m_sort += " DESC ";

    if (CollectionDB::instance()->getType() == DbConnection::postgresql)
    {
        if (!m_values.isEmpty()) m_values += ',';
        if ( b ) m_values += "LOWER( ";
        m_values += tableName( table ) + '.';
        m_values += valueName( value );
        if ( b ) m_values += ')';
        m_values += " as __discard ";
    }

    m_linkTables |= table;
}

void
QueryBuilder::sortByFunction( int function, int table, qint64 value, bool descending )
{
    // This function should be used with the equivalent addReturnFunctionValue (with the same function on same values)
    // since it uses the "func(table.value) AS functablevalue" definition.

    // this column is already coalesced, but need to reconstruct for postgres
    bool defaults = function == funcAvg && ( value & valScore || value & valRating );

    //shall we sort case-sensitively? (not for integer columns!)
    bool b = true;
    if ( value & valID || value & valTrack || value & valScore || value & valRating || value & valLength || value & valBitrate ||
         value & valSamplerate || value & valPlayCounter || value & valAccessDate || value & valCreateDate ||
         value & valFilesize || value & valDiscNumber ||
         table & tabYear )
        b = false;

    // only coalesce for certain columns
    bool c = false;
    if ( !defaults && ( value & valScore || value & valRating || value & valPlayCounter || value & valAccessDate || value & valCreateDate ) )
        c = true;

    if ( !m_sort.isEmpty() ) m_sort += ',';
    //m_sort += functionName( function ) + '(';
    if ( b ) m_sort += "LOWER( ";
    if ( c && CollectionDB::instance()->getType() != DbConnection::mysql) m_sort += "COALESCE( ";

    QString columnName;

    if (CollectionDB::instance()->getType() == DbConnection::postgresql)
    {
        columnName = functionName( function ) + '(';
        if ( defaults )
            columnName += "COALESCE(NULLIF(";
        columnName += tableName( table )+'.'+valueName( value );
        if ( defaults )
        {
            columnName += ", 0), ";
            if ( value & valScore )
                columnName += "50";
            else
                columnName += '6';
            columnName += ')';
        }
        columnName += ')';
    }
    else
        columnName = functionName( function )+tableName( table )+valueName( value );

    m_sort += columnName;

    if ( c && CollectionDB::instance()->getType() != DbConnection::mysql) m_sort += ", 0 )";

    if ( b ) m_sort += " ) ";
    //m_sort += " ) ";
    if ( descending ) m_sort += " DESC ";

    m_linkTables |= table;
}

void
QueryBuilder::groupBy( int table, qint64 value )
{
    if ( !m_group.isEmpty() ) m_group += ',';

    //Do case-sensitive comparisons for MySQL too. See also QueryBuilder::addReturnValue
    if ( DbConnection::mysql == CollectionDB::instance()->getDbConnectionType() &&
         ( value == valName || value == valTitle || value == valComment ) )
    {
        m_group += "BINARY ";
    }

    m_group += tableName( table ) + '.';
    m_group += valueName( value );

    m_linkTables |= table;
}

void
QueryBuilder::having( int table, qint64 value, int function, int mode, const QString& match )
{
    if( !m_having.isEmpty() ) m_having += " AND ";

    QString fn = functionName( function );
    fn.isEmpty() ?
        m_having += tableName( table ) + '.' + valueName( value ) :
        m_having += functionName( function )+'('+tableName( table )+'.'+valueName( value )+')';

    switch( mode )
    {
        case modeNormal:
            m_having += '=' + match;
            break;

        case modeLess:
            m_having += '<' + match;
            break;

        case modeGreater:
            m_having += '>' + match;

        default:
            break;
    }
}

void
QueryBuilder::setLimit( int startPos, int length )
{
    m_limit = QString( " LIMIT %2 OFFSET %1 " ).arg( startPos ).arg( length );
}

void
QueryBuilder::shuffle( int table, qint64 value )
{
    if ( !m_sort.isEmpty() ) m_sort += " ,  ";
    if ( table == 0 || value == 0 ) {
        // simple random
        m_sort += CollectionDB::instance()->randomFunc();
    } else {
        // This is the score weighted random order.

        // The RAND() function returns random values equally distributed between 0.0
        // (inclusive) and 1.0 (exclusive).  The obvious way to get this order is to
        // put every track <score> times into a list, sort the list by RAND()
        // (i.e. shuffle it) and discard every occurrence of every track but the very
        // first of each.  By putting every track into the list only once but applying
        // a transfer function T_s(x) := 1-(1-x)^(1/s) where s is the score, to RAND()
        // before sorting the list, exactly the same distribution of tracks can be
        // achieved (for a proof write to Stefan Siegel <kde@sdas.de>)

        // In the query below a simplified function is used: The score is incremented
        // by one to prevent division by zero, RAND() is used instead of 1-RAND()
        // because it doesn't matter if it becomes zero (the exponent is always
        // non-zero), and finally POWER(...) is used instead of 1-POWER(...) because it
        // only changes the order type.
        m_sort += QString("POWER( %1, 1.0 / (%2.%3 + 1) ) DESC")
            .arg( CollectionDB::instance()->randomFunc() )
            .arg( tableName( table ) )
            .arg( valueName( value ) );

        m_linkTables |= table;
    }
}


/* NOTE: It's important to keep these two functions and the const in sync! */
/* NOTE: It's just as important to keep tags.url first! */
const int
QueryBuilder::dragFieldCount = 21;

QString
QueryBuilder::dragSQLFields()
{
    return "tags.url, tags.deviceid, album.name, artist.name, composer.name, "
           "genre.name, tags.title, year.name, "
           "tags.comment, tags.track, tags.bitrate, tags.discnumber, "
           "tags.length, tags.samplerate, tags.filesize, "
           "tags.sampler, tags.filetype, tags.bpm, "
           "statistics.percentage, statistics.rating, statistics.playcounter, "
           "statistics.accessdate";
}

void
QueryBuilder::initSQLDrag()
{
    clear();
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabComposer, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valComment );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTrack );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valBitrate );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valLength );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valSamplerate );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFilesize );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valIsCompilation );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFileType );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valBPM );
    addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
    addReturnValue( QueryBuilder::tabStats, QueryBuilder::valRating );
    addReturnValue( QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
    addReturnValue( QueryBuilder::tabStats, QueryBuilder::valAccessDate );
}


void
QueryBuilder::buildQuery( bool withDeviceidPlaceholder )
{
    if ( m_query.isEmpty() )
    {
        linkTables( m_linkTables );
        m_query += "SELECT ";
        m_query += m_values;
        m_query += " FROM ";
        m_query += m_tables;
        m_query += ' ';
        m_query += m_join;
        m_query += " WHERE ";
        m_query += CollectionDB::instance()->boolT();
        m_query += ' ';
        m_query += m_where;
        if ( !m_showAll && ( m_linkTables & tabSong || m_tables.contains( tableName( tabSong) ) ) )     //Only stuff on mounted devices, unless you use optShowAll
        {
            if ( withDeviceidPlaceholder )
                m_query += "(*MountedDeviceSelection*)";
            else
            {
                IdList list = MountPointManager::instance()->getMountedDeviceIds();
                //debug() << "number of device ids " << list.count() << endl;
                m_query += " AND tags.deviceid IN (";
                oldForeachType( IdList, list )
                {
                    if ( it != list.begin() ) m_query += ',';
                    m_query += QString::number( *it );
                }
                m_query += ')';
            }
        }
        // GROUP BY must be before ORDER BY for sqlite
        // HAVING must be between GROUP BY and ORDER BY
        if ( !m_group.isEmpty() )  { m_query += " GROUP BY "; m_query += m_group; }
        if ( !m_having.isEmpty() ) { m_query += " HAVING "; m_query += m_having; }
        if ( !m_sort.isEmpty() )   { m_query += " ORDER BY "; m_query += m_sort; }
        m_query += m_limit;
        m_query += ';';
    }
}

// get the builded SQL-Query (used in smartplaylisteditor soon)
QString
QueryBuilder::getQuery()
{
    if ( m_query.isEmpty())
    {
        buildQuery();
    }
    return m_query;
}

QStringList
QueryBuilder::run()
{
    buildQuery();
    //debug() << m_query << endl;
    QStringList rs = CollectionDB::instance()->query( m_query );
    //calling code is unaware of the dynamic collection implementation, it simply expects an URL
    if( m_deviceidPos > 0 )
    {
        return cleanURL( rs );
    }
    else
        return rs;
}


void
QueryBuilder::clear()
{
    m_query.setLength(0);
    m_values.setLength(0);
    m_tables.setLength(0);
    m_join.setLength(0);
    m_where.setLength(0);
    m_sort.setLength(0);
    m_group.setLength(0);
    m_limit.setLength(0);
    m_having.setLength(0);

    m_linkTables = 0;
    m_returnValues = 0;

    m_showAll = false;
    m_deviceidPos = 0;
}


qint64
QueryBuilder::valForFavoriteSorting() {
    qint64 favSortBy = valRating;
    if ( !AmarokConfig::useScores() && !AmarokConfig::useRatings() )
        favSortBy = valPlayCounter;
    else if( !AmarokConfig::useRatings() )
        favSortBy = valScore;
    return favSortBy;
}

void
QueryBuilder::sortByFavorite() {
    if ( AmarokConfig::useRatings() )
        sortBy(tabStats, valRating, true );
    if ( AmarokConfig::useScores() )
        sortBy(tabStats, valScore, true );
    sortBy(tabStats, valPlayCounter, true );

}

void
QueryBuilder::sortByFavoriteAvg() {
    // Due to MySQL4 weirdness, we need to add the function we're using to sort
    // as return values as well.
    if ( AmarokConfig::useRatings() ) {
        sortByFunction(funcAvg, tabStats, valRating, true );
        addReturnFunctionValue( funcAvg, tabStats, valRating );
    }
    if ( AmarokConfig::useScores() ) {
        sortByFunction(funcAvg, tabStats, valScore, true );
        addReturnFunctionValue( funcAvg, tabStats, valScore );
    }
    sortByFunction(funcAvg, tabStats, valPlayCounter, true );
    addReturnFunctionValue( funcAvg, tabStats, valPlayCounter );

    //exclude unrated and unplayed
    if( !m_having.isEmpty() )
        m_having += " AND ";
    m_having += " (";
    if (AmarokConfig::useRatings() )
        m_having += QString("%1(%2.%3) > 0 OR ")
                   .arg( functionName( funcAvg ), tableName(tabStats), valueName(valRating) );
    m_having += QString("%1(%2.%3) > 0")
                   .arg( functionName( funcAvg ), tableName(tabStats), valueName(valPlayCounter) );
    m_having += ')';
}

// Helper method -- given a value, returns the index of the bit that is
// set, if only one, otherwise returns -1
// Binsearch seems appropriate since the values enum has 40 members
template<class ValueType>
static inline int
searchBit( ValueType value, int numBits ) {
   int low = 0, high = numBits - 1;
   while( low <= high ) {
       int mid = (low + high) / 2;
       ValueType compare = static_cast<ValueType>( 1 ) << mid;
       if ( value == compare ) return mid;
       else if ( value < compare ) high = mid - 1;
       else low = mid + 1;
   }

   return -1;
}

QString
QueryBuilder::tableName( int table )
{
    // optimize for 1 table which is by far the most frequent case
    static const QString tabNames[] = {
        "album",
        "artist",
        "composer",
        "genre",
        "year",
        "<unused>",             // 32 is missing from the enum
        "tags",
        "statistics",
        "lyrics",
        "podcastchannels",
        "podcastepisodes",
        "podcasttables",
        "devices",
        "labels"
    };

    int oneBit = searchBit( table, sizeof( tabNames ) / sizeof( QString ) );
    if ( oneBit >= 0 ) return tabNames[oneBit];

    // slow path: multiple tables. This seems to be unneeded at the moment,
    // but leaving it here since it appears to be intended usage
    QString tables;

    if ( CollectionDB::instance()->getType() != DbConnection::postgresql )
    {
        if ( table & tabSong )   tables += ",tags";
    }
    if ( table & tabArtist ) tables += ",artist";
    if ( table & tabComposer ) tables += ",composer";
    if ( table & tabAlbum )  tables += ",album";
    if ( table & tabGenre )  tables += ",genre";
    if ( table & tabYear )   tables += ",year";
    if ( table & tabStats )  tables += ",statistics";
    if ( table & tabLyrics )  tables += ",lyrics";
    if ( table & tabPodcastChannels ) tables += ",podcastchannels";
    if ( table & tabPodcastEpisodes ) tables += ",podcastepisodes";
    if ( table & tabPodcastFolders ) tables += ",podcasttables";
    if ( CollectionDB::instance()->getType() == DbConnection::postgresql )
    {
        if ( table & tabSong )   tables += ",tags";
    }

    if ( table & tabDevices ) tables += ",devices";
    if ( table & tabLabels ) tables += ",labels";
    // when there are multiple tables involved, we always need table tags for linking them
    return tables.mid( 1 );
}


const QString &
QueryBuilder::valueName( qint64 value )
{
   static const QString values[] = {
       "id",
       "name",
       "url",
       "title",
       "track",
       "percentage",
       "comment",
       "bitrate",
       "length",
       "samplerate",
       "playcounter",
       "createdate",
       "accessdate",
       "percentage",
       "artist",
       "album",
       "year",
       "genre",
       "dir",
       "lyrics",
       "rating",
       "composer",
       "discnumber",
       "filesize",
       "filetype",
       "sampler",
       "bpm",
       "copyright",
       "parent",
       "weblink",
       "autoscan",
       "fetchtype",
       "autotransfer",
       "haspurge",
       "purgeCount",
       "isNew",
       "deviceid",
       "url",
       "label",
       "lastmountpoint",
       "type"
   };

   int oneBit = searchBit( value, sizeof( values ) / sizeof( QString ) );
   if ( oneBit >= 0 ) return values[oneBit];

   static const QString error( "<ERROR valueName>" );
   return error;
}

/*
 * Return true if we should call COALESCE(..,0) for this DB field
 * (field names sourced from the old smartplaylistbrowser.cpp code)
 * Warning: addFilter( int, qint64, const QString&, int bool )
 * expects this method to return true for all statistics table clomuns of type INTEGER
 * Sqlite doesn't like comparing strings to an INTEGER column.
 */
bool
QueryBuilder::coalesceField( int table, qint64 value )
{
    if( tableName( table ) == "statistics" &&
        ( valueName( value ) == "playcounter" ||
          valueName( value ) == "rating" ||
          valueName( value ) == "percentage" ||
          valueName( value ) == "accessdate" ||
          valueName( value ) == "createdate"
        )
    )
       return true;
   return false;
}

QString
QueryBuilder::functionName( int function )
{
    QString functions;

    if ( function & funcCount )     functions += "Count";
    if ( function & funcMax )       functions += "Max";
    if ( function & funcMin )       functions += "Min";
    if ( function & funcAvg )       functions += "Avg";
    if ( function & funcSum )       functions += "Sum";

    return functions;
}

// FIXME: the two functions below are inefficient, but this patch is getting too
// big already. They are not on any critical path right now. Ovy
int
QueryBuilder::getTableByName(const QString &name)
{
    for ( int i = 1; i <= tabLabels; i <<= 1 )
    {
        if (tableName(i) == name) return i;
    }
    return -1;
}

qint64
QueryBuilder::getValueByName(const QString &name)
{
    for ( qint64 i = 1; i <= valType; i <<= 1 ) {
        if (valueName(i) == name) return i;
    }

    return -1;
}

bool
QueryBuilder::getField(const QString &tableValue, int *table, qint64 *value)
{
    int dotIndex = tableValue.find( '.' ) ;
    if ( dotIndex < 0 ) return false;
    int tmpTable = getTableByName( tableValue.left(dotIndex) );
    quint64 tmpValue = getValueByName( tableValue.mid( dotIndex + 1 ) );
    if ( tmpTable >= 0 && value ) {
        *table = tmpTable;
        *value = tmpValue;
        return true;
    }
    else
    {
        qFatal("invalid table.value: %s", tableValue.toAscii());
        return false;
    }
}



QStringList
QueryBuilder::cleanURL( QStringList result )
{
    //this method replaces the fields for relative path and devive/media id with a
    //single field containing the absolute path for each row
    int count = 1;
    for( QMutableStringListIterator iter( result ); iter.hasNext(); )
    {
        QString rpath;
        if ( (count % (m_returnValues + 1)) + 1== m_deviceidPos )
        {
            //this block is reached when the iterator points at the relative path
            //deviceid is next
            QString rpath = iter.next();
            int deviceid = iter.peekNext().toInt();
            QString abspath = MountPointManager::instance()->getAbsolutePath( deviceid, rpath );
            iter.setValue( abspath );
            iter.next();
            iter.remove();
            //we advanced the iterator over two fields in this iteration
            ++count;
        }
        else
            iter.next();
        ++count;
    }
    return result;
}
