/***************************************************************************
 *   Copyright (C)  2004-2005 Mark Kretschmann <markey@web.de>             *
 *                  2004 Christian Muehlhaeuser <chris@chris.de>           *
 *                  2004 Sami Nieminen <sami.nieminen@iki.fi>              *
 *                  2005 Ian Monroe <ian@monroe.nu>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <dbenginebase.h>
#include <qstringlist.h>

#include <klocale.h>


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS DBConnection
//////////////////////////////////////////////////////////////////////////////////////////

DbConnection::DbConnection( DbConfig* config )
    : m_config( config )
{}


DbConnection::~DbConnection()
{}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS QueryBuilder
//////////////////////////////////////////////////////////////////////////////////////////

QueryBuilder::QueryBuilder()
{
    clear();
}


void
QueryBuilder::linkTables( int tables )
{

    m_tables = tableName( tabSong );

    if ( !(tables & tabSong ) )
    {
        // check if only one table is selected (does somebody know a better way to check that?)
        if (tables == tabAlbum || tables==tabArtist || tables==tabGenre || tables == tabYear || tables == tabStats)
            m_tables = tableName(tables);
        else
            tables |= tabSong;
    }


    if ( tables & tabSong )
    {
        if ( tables & tabAlbum )
            m_tables += " INNER JOIN " + tableName( tabAlbum) + " ON album.id=tags.album";
        if ( tables & tabArtist )
            m_tables += " INNER JOIN " + tableName( tabArtist) + " ON artist.id=tags.artist";
        if ( tables & tabGenre )
            m_tables += " INNER JOIN " + tableName( tabGenre) + " ON genre.id=tags.genre";
        if ( tables & tabYear )
            m_tables += " INNER JOIN " + tableName( tabYear) + " ON year.id=tags.year";
        if ( tables & tabStats )
            m_tables += " INNER JOIN " + tableName( tabStats) + " ON statistics.url=tags.url";
    }
}


void
QueryBuilder::addReturnValue( int table, int value )
{
    if ( !m_values.isEmpty() && m_values != "DISTINCT " ) m_values += ",";
    if ( table & tabStats && value & valScore ) m_values += "round(";

    if ( value == valDummy )
        m_values += "''";
    else
    {
        m_values += tableName( table ) + ".";
        m_values += valueName( value );
    }

    if ( table & tabStats && value & valScore ) m_values += " + 0.4 )";

    m_linkTables |= table;
    m_returnValues++;
}

void
QueryBuilder::addReturnFunctionValue( int function, int table, int value)
{
    if ( !m_values.isEmpty() && m_values != "DISTINCT " ) m_values += ",";
    m_values += functionName( function ) + "(";
    m_values += tableName( table ) + ".";
    m_values += valueName( value )+ ")";
    m_values += " AS ";
    m_values += functionName( function )+tableName( table )+valueName( value );

    m_linkTables |= table;
    m_returnValues++;
}

uint
QueryBuilder::countReturnValues()
{
    return m_returnValues;
}


void
QueryBuilder::addURLFilters( const QStringList& filter )
{
    if ( !filter.isEmpty() )
    {
        m_where += "AND ( true ";

        for ( uint i = 0; i < filter.count(); i++ )
        {
                m_where += "OR tags.url = '" + escapeString( filter[i] ) + "' ";
        }

        m_where += " ) ";
    }

    m_linkTables |= tabSong;
}


void
QueryBuilder::addFilter( int tables, const QString& filter, int /*mode*/ )
{
    if ( !filter.isEmpty() )
    {
        m_where += "AND ( true ";
        if ( tables & tabAlbum ) m_where += "OR album.name LIKE '%" + escapeString( filter ) + "%' ";
        if ( tables & tabArtist ) m_where += "OR artist.name LIKE '%" + escapeString( filter ) + "%' ";
        if ( tables & tabGenre ) m_where += "OR genre.name LIKE '%" + escapeString( filter ) + "%' ";
        if ( tables & tabYear ) m_where += "OR year.name LIKE '%" + escapeString( filter ) + "%' ";
        if ( tables & tabSong ) m_where += "OR tags.title LIKE '%" + escapeString( filter ) + "%' ";
        m_where += " ) ";
    }

    m_linkTables |= tables;
}


void
QueryBuilder::addFilters( int tables, const QStringList& filter )
{
    if ( !filter.isEmpty() )
    {
        m_where += "AND ( true ";

        for ( uint i = 0; i < filter.count(); i++ )
        {
            m_where += " AND ( true ";
            if ( tables & tabAlbum ) m_where += "OR album.name LIKE '%" + escapeString( filter[i] ) + "%' ";
            if ( tables & tabArtist ) m_where += "OR artist.name LIKE '%" + escapeString( filter[i] ) + "%' ";
            if ( tables & tabGenre ) m_where += "OR genre.name LIKE '%" + escapeString( filter[i] ) + "%' ";
            if ( tables & tabYear ) m_where += "OR year.name LIKE '%" + escapeString( filter[i] ) + "%' ";
            if ( tables & tabSong ) m_where += "OR tags.title LIKE '%" + escapeString( filter[i] ) + "%' ";
            m_where += " ) ";
        }

        m_where += " ) ";
    }

    m_linkTables |= tables;
}


void
QueryBuilder::addMatch( int tables, const QString& match )
{
    if ( !match.isEmpty() )
    {
        m_where += "AND ( true ";
        if ( tables & tabAlbum ) m_where += "OR album.name LIKE '" + escapeString( match ) + "' ";
        if ( tables & tabArtist ) m_where += "OR artist.name LIKE '" + escapeString( match ) + "' ";
        if ( tables & tabGenre ) m_where += "OR genre.name LIKE '" + escapeString( match ) + "' ";
        if ( tables & tabYear ) m_where += "OR year.name LIKE '" + escapeString( match ) + "' ";
        if ( tables & tabSong ) m_where += "OR tags.title LIKE '" + escapeString( match ) + "' ";

        if ( match == i18n( "Unknown" ) )
        {
            if ( tables & tabAlbum ) m_where += "OR album.name = '' ";
            if ( tables & tabArtist ) m_where += "OR artist.name = '' ";
            if ( tables & tabGenre ) m_where += "OR genre.name = '' ";
            if ( tables & tabYear ) m_where += "OR year.name = '' ";
        }
        m_where += " ) ";
    }

    m_linkTables |= tables;
}


void
QueryBuilder::addMatch( int tables, int value, const QString& match )
{
    if ( !match.isEmpty() )
    {
        m_where += "AND ( true ";
        m_where += QString( "OR %1.%2 LIKE '" ).arg( tableName( tables ) ).arg( valueName( value ) ) + escapeString( match ) + "' ";

        if ( ( value & valName ) && match == i18n( "Unknown" ) )
            m_where += QString( "OR %1.%2 = '' " ).arg( tableName( tables ) ).arg( valueName( value ) );

        m_where += " ) ";
    }

    m_linkTables |= tables;
}


void
QueryBuilder::addMatches( int tables, const QStringList& match )
{
    if ( !match.isEmpty() )
    {
        m_where += "AND ( true ";

        for ( uint i = 0; i < match.count(); i++ )
        {
            if ( tables & tabAlbum ) m_where += "OR album.name LIKE '" + escapeString( match[i] ) + "' ";
            if ( tables & tabArtist ) m_where += "OR artist.name LIKE '" + escapeString( match[i] ) + "' ";
            if ( tables & tabGenre ) m_where += "OR genre.name LIKE '" + escapeString( match[i] ) + "' ";
            if ( tables & tabYear ) m_where += "OR year.name LIKE '" + escapeString( match[i] ) + "' ";
            if ( tables & tabSong ) m_where += "OR tags.title LIKE '" + escapeString( match[i] ) + "' ";
            if ( tables & tabStats ) m_where += "OR statistics.url LIKE '" + escapeString( match[i] ) + "' ";

            if ( match[i] == i18n( "Unknown" ) )
            {
                if ( tables & tabAlbum ) m_where += "OR album.name = '' ";
                if ( tables & tabArtist ) m_where += "OR artist.name = '' ";
                if ( tables & tabGenre ) m_where += "OR genre.name = '' ";
                if ( tables & tabYear ) m_where += "OR year.name = '' ";
            }
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
        m_where += "AND ( true ";
        if ( tables & tabAlbum ) m_where += "AND album.name <> '%" + escapeString( filter ) + "%' ";
        if ( tables & tabArtist ) m_where += "AND artist.name <> '%" + escapeString( filter ) + "%' ";
        if ( tables & tabGenre ) m_where += "AND genre.name <> '%" + escapeString( filter ) + "%' ";
        if ( tables & tabYear ) m_where += "AND year.name <> '%" + escapeString( filter ) + "%' ";
        if ( tables & tabSong ) m_where += "AND tags.title <> '%" + escapeString( filter ) + "%' ";
        m_where += " ) ";
    }

    m_linkTables |= tables;
}


void
QueryBuilder::excludeMatch( int tables, const QString& match )
{
    if ( !match.isEmpty() )
    {
        m_where += "AND ( true ";
        if ( tables & tabAlbum ) m_where += "AND album.name <> '" + escapeString( match ) + "' ";
        if ( tables & tabArtist ) m_where += "AND artist.name <> '" + escapeString( match ) + "' ";
        if ( tables & tabGenre ) m_where += "AND genre.name <> '" + escapeString( match ) + "' ";
        if ( tables & tabYear ) m_where += "AND year.name <> '" + escapeString( match ) + "' ";
        if ( tables & tabSong ) m_where += "AND tags.title <> '" + escapeString( match ) + "' ";

        if ( match == i18n( "Unknown" ) )
        {
            if ( tables & tabAlbum ) m_where += "AND album.name <> '' ";
            if ( tables & tabArtist ) m_where += "AND artist.name <> '' ";
            if ( tables & tabGenre ) m_where += "AND genre.name <> '' ";
            if ( tables & tabYear ) m_where += "AND year.name <> '' ";
        }
        m_where += " ) ";
    }

    m_linkTables |= tables;
}


void
QueryBuilder::exclusiveFilter( int tableMatching, int tableNotMatching, int value )
{
    m_join += " LEFT JOIN ";
    m_join += tableName( tableNotMatching );
    m_join += " ON ";

    m_join += tableName( tableMatching ) + ".";
    m_join += valueName( value );
    m_join+= " = ";
    m_join += tableName( tableNotMatching ) + ".";
    m_join += valueName( value );

    m_where += " AND ";
    m_where += tableName( tableNotMatching ) + ".";
    m_where += valueName( value );
    m_where += " IS null ";
}


void
QueryBuilder::setOptions( int options )
{
    if ( options & optNoCompilations || options & optOnlyCompilations )
        m_linkTables |= tabSong;

    if ( options & optNoCompilations ) m_where += "AND tags.sampler = 0 ";
    if ( options & optOnlyCompilations ) m_where += "AND tags.sampler = 1 ";

    if ( options & optRemoveDuplicates ) m_values = "DISTINCT " + m_values;
    if ( options & optRandomize )
    {
        if ( !m_sort.isEmpty() ) m_sort += ",";
        m_sort += "RAND() ";
    }
}


void
QueryBuilder::sortBy( int table, int value, bool descending )
{
    //shall we sort case-sensitively? (not for integer columns!)
    bool b = true;
    if ( value & valID || value & valTrack || value & valScore || value & valLength || value & valBitrate ||
         value & valSamplerate || value & valPlayCounter || value & valAccessDate || value & valCreateDate || value & valPercentage ||
         table & tabYear )
        b = false;

    if ( !m_sort.isEmpty() ) m_sort += ",";
    if ( b ) m_sort += "LOWER( ";
    if ( table & tabYear ) m_sort += "(";

    m_sort += tableName( table ) + ".";
    m_sort += valueName( value );

    if ( table & tabYear ) m_sort += "+0)";

    if ( b ) m_sort += " ) ";
    if ( descending ) m_sort += " DESC ";

    m_linkTables |= table;
}

void
QueryBuilder::sortByFunction( int function, int table, int value, bool descending )
{
    // This function should be used with the equivalent addReturnFunctionValue (with the same function on same values)
    // since it uses the "func(table.value) AS functablevalue" definition.

    //shall we sort case-sensitively? (not for integer columns!)
    bool b = true;
    if ( value & valID || value & valTrack || value & valScore || value & valLength || value & valBitrate ||
         value & valSamplerate || value & valPlayCounter || value & valAccessDate || value & valCreateDate || value & valPercentage ||
         table & tabYear )
        b = false;

    if ( !m_sort.isEmpty() ) m_sort += ",";
    //m_sort += functionName( function ) + "(";
    if ( b ) m_sort += "LOWER( ";
    if ( table & tabYear ) m_sort += "(";

    QString columnName = functionName( function )+tableName( table )+valueName( value );
    m_sort += columnName;

    if ( table & tabYear ) m_sort += "+0)";
    if ( b ) m_sort += " ) ";
    //m_sort += " ) ";
    if ( descending ) m_sort += " DESC ";

    m_linkTables |= table;
}

void
QueryBuilder::groupBy( int table, int value )
{
    if ( !m_group.isEmpty() ) m_group += ",";
    m_group += tableName( table ) + ".";
    m_group += valueName( value );

    m_linkTables |= table;
}


void
QueryBuilder::setLimit( int startPos, int length )
{
    m_limit = QString( " LIMIT %1, %2 " ).arg( startPos ).arg( length );
}


void
QueryBuilder::initSQLDrag()
{
    clear();
    addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valComment );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTrack );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valBitrate );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valLength );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valSamplerate );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
}


void
QueryBuilder::buildQuery()
{
    if ( m_query.isEmpty() )
    {
        linkTables( m_linkTables );

        m_query = "SELECT " + m_values + " FROM " + m_tables + " " + m_join + " WHERE true " + m_where;
        // GROUP BY must be before ORDER BY for sqlite
        if ( !m_group.isEmpty() ) m_query += " GROUP BY " + m_group;
        if ( !m_sort.isEmpty() ) m_query += " ORDER BY " + m_sort;
        m_query += m_limit;
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
//    return query( m_query );
}


void
QueryBuilder::clear()
{
    m_query = "";
    m_values = "";
    m_tables = "";
    m_join = "";
    m_where = "";
    m_sort = "";
    m_group = "";
    m_limit = "";

    m_linkTables = 0;
    m_returnValues = 0;
}


QString
QueryBuilder::tableName( int table )
{
    QString tables;

    if ( table & tabSong )   tables += ",tags";
    if ( table & tabArtist ) tables += ",artist";
    if ( table & tabAlbum )  tables += ",album";
    if ( table & tabGenre )  tables += ",genre";
    if ( table & tabYear )   tables += ",year";
    if ( table & tabStats )  tables += ",statistics";

    // when there are multiple tables involved, we always need table tags for linking them
    return tables.mid( 1 );
}


QString
QueryBuilder::valueName( int value )
{
    QString values;

    if ( value & valID )          values += "id";
    if ( value & valName )        values += "name";
    if ( value & valURL )         values += "url";
    if ( value & valTitle )       values += "title";
    if ( value & valTrack )       values += "track";
    if ( value & valScore )       values += "percentage";
    if ( value & valComment )     values += "comment";
    if ( value & valBitrate )     values += "bitrate";
    if ( value & valLength )      values += "length";
    if ( value & valSamplerate )  values += "samplerate";
    if ( value & valPlayCounter ) values += "playcounter";
    if ( value & valAccessDate )  values += "accessdate";
    if ( value & valCreateDate )  values += "createdate";
    if ( value & valPercentage )  values += "percentage";
    if ( value & valArtistID )    values += "artist";
    if ( value & valAlbumID )     values += "album";
    if ( value & valGenreID )     values += "genre";
    if ( value & valYearID )      values += "year";

    return values;
}

QString
QueryBuilder::functionName( int value )
{
    QString function;

    if ( value & funcCount )     function += "Count";
    if ( value & funcMax )       function += "Max";
    if ( value & funcMin )       function += "Min";
    if ( value & funcAvg )       function += "Avg";
    if ( value & funcSum )       function += "Sum";

    return function;
}

