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

#ifndef AMAROK_DBENGINEBASE_H
#define AMAROK_DBENGINEBASE_H

#include "plugin/plugin.h" //baseclass
#include <qobject.h>       //baseclass


class DbConfig
{};

class DbConnection : public QObject, public amaroK::Plugin
{
    public:
        enum DbConnectionType { sqlite = 0, mysql = 1, postgresql = 2 };

        DbConnection( DbConfig* /* config */ );
        virtual ~DbConnection() = 0;

        virtual QStringList query( const QString& /* statement */ ) = 0;
        virtual int insert( const QString& /* statement */, const QString& /* table */ ) = 0;
        const bool isInitialized() const { return m_initialized; }
        virtual bool isConnected() const = 0;
        virtual const QString lastError() const { return "None"; }

    protected:
        bool m_initialized;
        DbConfig *m_config;
};


class QueryBuilder
{
    public:
        //attributes:
        enum qBuilderTables  { tabAlbum = 1, tabArtist = 2, tabGenre = 4, tabYear = 8, tabSong = 32, tabStats = 64, tabDummy = 0 };
        enum qBuilderOptions { optNoCompilations = 1, optOnlyCompilations = 2, optRemoveDuplicates = 4, optRandomize = 8 };
        enum qBuilderValues  { valID = 1, valName = 2, valURL = 4, valTitle = 8, valTrack = 16, valScore = 32, valComment = 64,
                               valBitrate = 128, valLength = 256, valSamplerate = 512, valPlayCounter = 1024,
                               valCreateDate = 2048, valAccessDate = 4096, valPercentage = 8192, valArtistID = 16384, valAlbumID = 32768,
                               valYearID = 65536, valGenreID = 131072, valDummy = 0 };
        enum qBuilderFunctions  { funcCount = 1, funcMax = 2, funcMin = 4, funcAvg = 8, funcSum = 16 };

        enum qBuilderFilter  { modeNormal = 0, modeFuzzy = 1 };

        QueryBuilder();

        QString escapeString( QString string )
        {
            return
                #ifdef USE_MYSQL
                    // We have to escape "\" for mysql, but can't do so for sqlite
                    (m_dbConnType == DbConnection::mysql)
                            ? string.replace("\\", "\\\\").replace( '\'', "''" )
                            :
                #endif
                    string.replace( '\'', "''" );
        }

        void addReturnValue( int table, int value );
        void addReturnFunctionValue( int function, int table, int value);
        uint countReturnValues();

        void addURLFilters( const QStringList& filter );

        void addFilter( int tables, const QString& filter, int mode = modeNormal );
        void addFilters( int tables, const QStringList& filter );
        void excludeFilter( int tables, const QString& filter );

        void addMatch( int tables, const QString& match );
        void addMatch( int tables, int value, const QString& match );
        void addMatches( int tables, const QStringList& match );
        void excludeMatch( int tables, const QString& match );

        void exclusiveFilter( int tableMatching, int tableNotMatching, int value );

        void setOptions( int options );
        void sortBy( int table, int value, bool descending = false );
        void sortByFunction( int function, int table, int value, bool descending = false );
        void groupBy( int table, int value );
        void setLimit( int startPos, int length );

        void initSQLDrag();
        void buildQuery();
        QString getQuery();
        QString query() { buildQuery(); return m_query; };
        void clear();

        QStringList run();

    private:
        QString tableName( int table );
        QString valueName( int value );
        QString functionName( int value );

        void linkTables( int tables );

        QString m_query;
        QString m_values;
        QString m_tables;
        QString m_join;
        QString m_where;
        QString m_sort;
        QString m_group;
        QString m_limit;

        int m_linkTables;
        uint m_returnValues;
};


#endif /*AMAROK_DBENGINEBASE_H*/
