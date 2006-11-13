/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
 * copyright            : (C) 2006 Gábor Lehel <illissius@gmail.com>       *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************
 * Infiltrations of Dynamic Mode                                           *
 *   Dynamic mode is a complex playlist handling mechanism - acting        *
 *   basically on the concept of a 'rotating' playlist.  The playlist can  *
 *   be modelled as a queuing system, FIFO.  As a track is advanced,       *
 *   the first track in the playlist is removed, and another appended to   *
 *   the end.  The type of addition is selected by the user during         *
 *   configuration.                                                        *
 *                                                                         *
 *   Due to the nature of this type of handling, the status of dynamicmode *
 *   must be determined, as many function require alternate handling.      *
 *      Examples include:                                                  *
 *          - Context Menus                                                *
 *          - Double clicking on an item -> requires moving the item to    *
 *            front of the queue                                           *
 *          - Undo/Redo states, to reinit history items                    *
 *   Please be aware of these when working with dynamic mode.              *
 ***************************************************************************/

#ifndef AMAROK_DYNAMIC_H
#define AMAROK_DYNAMIC_H

#include <kurl.h>  //KURL::List

class QString;
class QStringList;
template<class T> class QPtrList;
class PlaylistBrowserEntry;
class PlaylistEntry;
class SmartPlaylist;

class DynamicMode
{
    public:
        DynamicMode( const QString &name );
        virtual ~DynamicMode();
        enum Type { RANDOM=0, SUGGESTION=1, CUSTOM=2 };

        void edit();
        void deleting();
        void setDynamicItems( QPtrList<PlaylistBrowserEntry>& newList );

        /**
         * Retrieves \p tracks from the cache, \p m_cachedItemSet
         */
        KURL::List retrieveTracks( const uint trackCount );

        /**
         * Creates a list of \p CACHE_SIZE urls, stored in \p m_cachedItemSet in order
         * to increase efficiency of dynamic mode population. This should be called
         * when the dynamic sources are changed, or the cache runs out of items
         */
        void rebuildCachedItemSet();

        QString title() const;
        QStringList items() const;
        bool  cycleTracks() const;
        int   upcomingCount() const;
        int   previousCount() const;
        int   appendType() const;

        void  setAppendType( int type );
        void  setCycleTracks( bool cycle );
        void  setItems( const QStringList &list );
        void  setUpcomingCount( int count );
        void  setPreviousCount( int count );
        void  setTitle( const QString& title );

    private:
        static const int CACHE_SIZE = 200; ///< the number of items to store in the cached set

        /**
         * Returns a list of \p songCount urls from \p item - to be stored as part of
         * the dynamic element cache, \p m_cachedItemSet
         *
         * This function will alter the sql statement of the item in order to return an
         * adequate subset of data after execution. Limits and ordering attributes
         * within the statement will be respected (to a certain extent).
         */
        KURL::List tracksFromSmartPlaylist( SmartPlaylist *item, uint songCount );

        /**
         * Returns a list of \p songCount urls from \p item - to be stored as part of
         * the dynamic element cache, \p m_cachedItemSet
         *
         * This function will return a random selection of elements from within the
         * playlist given, in order to give some diversity when rebuilding the cache.
         */
        KURL::List tracksFromStaticPlaylist( PlaylistEntry *item, uint songCount );

        /**
         * A list of urls which satisfy at least one of the dynamic mode sources. As tracks
         * are added to the playlist, they are removed from the cache. When the cache expires,
         * it should be rebuilt.
         *
         * The cache is used to reduce the number of database queries required when adding and
         * removing tracks from the playlist when using dynamic mode.
         */
        KURL::List  m_cachedItemSet;

        QStringList m_items;

        QString m_title;
        bool    m_cycle;
        int     m_upcoming;
        int     m_previous;
        int     m_appendType;
};

#endif //AMAROK_DYNAMIC_H
