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

#ifndef AMAROK_QUERYBUIDER_H
#define AMAROK_QUERYBUIDER_H

#include "collectiondb.h"

#include <QtGlobal>
#include <QString>
#include <QStringList>

#include <Q3ValueStack>


class QueryBuilder
{
    public:
        //attributes:
        enum qBuilderTables  { tabAlbum = 1, tabArtist = 2, tabComposer = 4, tabGenre = 8, tabYear = 16, tabSong = 64,
                               tabStats = 128, tabLyrics = 256, tabPodcastChannels = 512,
                               tabPodcastEpisodes = 1024, tabPodcastFolders = 2048,
                               tabDevices = 4096, tabLabels = 8192
                               /* dummy table for filtering */, tabDummy = 0 };
        enum qBuilderOptions { optNoCompilations = 1, optOnlyCompilations = 2, optRemoveDuplicates = 4,
                               optRandomize = 8,
                               optShowAll = 16 /* get all songs, not just mounted ones */ };
        /* This has been an enum in the past, but 32 bits wasn't enough anymore :-( */
        static const qint64 valDummy         = 0;
        static const qint64 valID            = 1LL << 0;
        static const qint64 valName          = 1LL << 1;
        static const qint64 valURL           = 1LL << 2;
        static const qint64 valTitle         = 1LL << 3;
        static const qint64 valTrack         = 1LL << 4;
        static const qint64 valScore         = 1LL << 5;
        static const qint64 valComment       = 1LL << 6;
        static const qint64 valBitrate       = 1LL << 7;
        static const qint64 valLength        = 1LL << 8;
        static const qint64 valSamplerate    = 1LL << 9;
        static const qint64 valPlayCounter   = 1LL << 10;
        static const qint64 valCreateDate    = 1LL << 11;
        static const qint64 valAccessDate    = 1LL << 12;
        //static const qint64 valPercentage    = 1LL << 13; // same as valScore
        static const qint64 valArtistID      = 1LL << 14;
        static const qint64 valAlbumID       = 1LL << 15;
        static const qint64 valYearID        = 1LL << 16;
        static const qint64 valGenreID       = 1LL << 17;
        static const qint64 valDirectory     = 1LL << 18;
        static const qint64 valLyrics        = 1LL << 19;
        static const qint64 valRating        = 1LL << 20;
        static const qint64 valComposerID    = 1LL << 21;
        static const qint64 valDiscNumber    = 1LL << 22;
        static const qint64 valFilesize      = 1LL << 23;
        static const qint64 valFileType      = 1LL << 24;
        static const qint64 valIsCompilation = 1LL << 25;
        static const qint64 valBPM           = 1LL << 26;
        // podcast relevant:
        static const qint64 valCopyright     = 1LL << 27;
        static const qint64 valParent        = 1LL << 28;
        static const qint64 valWeblink       = 1LL << 29;
        static const qint64 valAutoscan      = 1LL << 30;
        static const qint64 valFetchtype     = 1LL << 31;
        static const qint64 valAutotransfer  = 1LL << 32;
        static const qint64 valPurge         = 1LL << 33;
        static const qint64 valPurgeCount    = 1LL << 34;
        static const qint64 valIsNew         = 1LL << 35;
        // dynamic collection relevant:
        static const qint64 valDeviceId      = 1LL << 36;
        static const qint64 valRelativePath  = 1LL << 37;
        static const qint64 valDeviceLabel   = 1LL << 38;
        static const qint64 valMountPoint    = 1LL << 39;
        //label relevant
        static const qint64 valType         = 1LL << 40;

        static qint64 valForFavoriteSorting();
        void sortByFavorite();

        // sortByFavoriteAvg() add the average rating, if enabled, the average score, if enabled,
        // and the average playcounter as return values!
        void sortByFavoriteAvg();

        enum qBuilderFunctions  { funcNone = 0, funcCount = 1, funcMax = 2, funcMin = 4, funcAvg = 8, funcSum = 16 };

        // Note: modes beginMatch, endMatch are only supported for string filters
        // Likewise, modes between and notBetween are only supported for numeric filters
        enum qBuilderFilter  { modeNormal = 0, modeLess = 1, modeGreater = 2, modeEndMatch = 3, modeBeginMatch = 4, modeBetween = 5, modeNotBetween = 6};

        QueryBuilder();

        void addReturnValue( int table, qint64 value, bool caseSensitive = false /* unless value refers to a string */ );
        void addReturnFunctionValue( int function, int table, qint64 value);
        uint countReturnValues();

        // Note: the filter chain begins in AND mode
        void beginOR(); //filters will be ORed instead of ANDed
        void endOR();   //don't forget to end it!
        void beginAND(); // These do the opposite; for recursive and/or
        void endAND();

        void setGoogleFilter( int defaultTables, QString query );

        void addUrlFilters( const QStringList& filter );

        void addFilter( int tables, const QString& filter);
        void addFilter( int tables, qint64 value, const QString& filter, int mode = modeNormal, bool exact = false );
        void addFilters( int tables, const QStringList& filter );
        void excludeFilter( int tables, const QString& filter );
        void excludeFilter( int tables, qint64 value, const QString& filter, int mode = modeNormal, bool exact = false );

        void addMatch( int tables, const QString& match, bool interpretUnknown = true, bool caseSensitive = true );
        void addMatch( int tables, qint64 value, const QString& match, bool interpretUnknown = true, bool caseSensitive = true );
        void addMatches( int tables, const QStringList& match, bool interpretUnknown = true, bool caseSensitive = true );
        void excludeMatch( int tables, const QString& match );
        void having( int table, qint64 value, int function, int mode, const QString& match );

        void exclusiveFilter( int tableMatching, int tableNotMatching, qint64 value );

        // For numeric filters:
        // modeNormal means strict equality; modeBeginMatch and modeEndMatch are not
        // allowed; modeBetween needs a second value endRange
        void addNumericFilter(int tables, qint64 value, const QString &n,
                              int mode = modeNormal,
                              const QString &endRange = QString::null);

        void setOptions( int options );
        void sortBy( int table, qint64 value, bool descending = false );
        void sortByFunction( int function, int table, qint64 value, bool descending = false );
        void groupBy( int table, qint64 value );
        void setLimit( int startPos, int length );

        // Returns the results in random order.
        // If a \p table and \p value are specified, uses weighted random order on
        // that field.
        // The shuffle is cumulative with other sorts, but any sorts after this are
        // pointless because of the precision of the random function.
        void shuffle( int table = 0, qint64 value = 0 );

        static const int dragFieldCount;
        static QString dragSQLFields();
        void initSQLDrag();

        void buildQuery( bool withDeviceidPlaceholder = false );
        QString getQuery();
        //use withDeviceidPlaceholder = false if the query isn't run immediately (*CurrentTimeT*)
        //and replace (*MountedDeviceSelection*) with CollectionDB::instance()->deviceIdSelection()
        QString query( bool withDeviceidPlaceholder = false ) { buildQuery( withDeviceidPlaceholder ); return m_query; };
        void clear();

        QStringList run();

        // Transform a string table.value "field" into enum values
        // @return true if we succeeded
        bool getField(const QString &tableValue, int *table, qint64 *value);

    private:
        QString tableName( int table );
        const QString &valueName( qint64 value );
        QString functionName( int functions );
        bool coalesceField( int table, qint64 value );

        int getTableByName(const QString &name);
        qint64 getValueByName(const QString &field);

        QStringList cleanURL( QStringList result );

        void linkTables( int tables );

        Q3ValueStack<bool> m_OR;
        bool m_showAll;
        uint m_deviceidPos;

        QString ANDslashOR() const;

        QString m_query;
        QString m_values;
        QString m_tables;
        QString m_join;
        QString m_where;
        QString m_sort;
        QString m_group;
        QString m_limit;
        QString m_having;

        QString m_url;      //url is used as primary key and linkTables needs to do some special stuff with it

        int m_linkTables;
        uint m_returnValues;
};

inline void QueryBuilder::beginOR()
{
    m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + ' ';
    m_OR.push(true);
}
inline void QueryBuilder::endOR()
{
    m_where += " ) ";
    m_OR.pop();
}
inline void QueryBuilder::beginAND()
{
    m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolT() + ' ';
    m_OR.push(false);
}
inline void QueryBuilder::endAND()
{
    m_where += " ) ";
    m_OR.pop();
}
inline QString QueryBuilder::ANDslashOR() const { return m_OR.top() ? "OR" : "AND"; }

#endif
