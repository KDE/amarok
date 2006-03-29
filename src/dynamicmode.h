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

class QString;
class QStringList;
template<class T> class QPtrList;
class QListViewItem;

class DynamicMode
{
    public:
        DynamicMode( const QString &name );
        virtual ~DynamicMode();
        enum Type { RANDOM=0, SUGGESTION=1, CUSTOM=2 };

        void edit();
        void setDynamicItems(const QPtrList<QListViewItem>& newList);

    public: //accessors
        QString title() const;
        QStringList items() const;
        bool  cycleTracks() const;
        bool  markHistory() const;
        int   upcomingCount() const;
        int   previousCount() const;
        int   appendCount() const;
        int   appendType() const;

    public: //setters
        void  setTitle( const QString& title );
        void  setItems( const QStringList &list );
        void  setCycleTracks( bool cycle );
        void  setMarkHistory( bool mark );
        void  setUpcomingCount( int count );
        void  setPreviousCount( int count );
        void  setAppendCount( int count );
        void  setAppendType( int type );

    private:
        QStringList m_items;

        QString m_title;
        bool    m_cycle;
        bool    m_mark;
        int     m_upcoming;
        int     m_previous;
        int     m_appendCount;
        int     m_appendType;
};

#endif //AMAROK_DYNAMIC_H
