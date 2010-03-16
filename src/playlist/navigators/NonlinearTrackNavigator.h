/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_NONLINEARTRACKNAVIGATOR_H
#define AMAROK_NONLINEARTRACKNAVIGATOR_H

#include "TrackNavigator.h"

#include <QList>
#include <QSet>

namespace Playlist
{
    /**
     * Base class that offers some standard services for non-linear navigators:
     *   - Deterministic back/forward in history.
     *   - High-performance keep-in-sync with changes in the source model.
     *   - 'm_plannedItems', a standard way to plan ahead.
     *
     * The simplest use case for a child class is to provide an implementation of 'planOne()'.
     *
     * A slightly more advanced use case for a child class is to override
     * 'foorbarInsertedItems()', 'foobarRemovedItems()', and 'setCurrentItem()'.
     *
     * If a child class really needs to do something special, it can override
     * 'requestNextTrack()' etc., as long as it tries the return value of the base
     * implementation first.
     */
    class NonlinearTrackNavigator : public TrackNavigator
    {
        Q_OBJECT

        public:
            NonlinearTrackNavigator();

            //! Overrides from 'TrackNavigator'
            quint64 likelyNextTrack();
            quint64 requestNextTrack();
            quint64 requestUserNextTrack() { return requestNextTrack(); }

            quint64 likelyLastTrack();
            quint64 requestLastTrack();

            static const int MAX_HISTORY_SIZE = 1000;    // This is probably enough for even the most crazed user.

            /**
             * 'setCurrentItem()' is guaranteed to be called whenever an item is removed from 'm_plannedItems'.
             */
            quint64 currentItem();
            virtual void setCurrentItem( const quint64 newItem, bool goingBackward = false );

            /**
             * All items currently in the source model.
             *
             * The QList is not kept in the same order as the source model. We could add
             * that guarantee, but currently no navigator needs it.
             *
             * The current reason for the QList variant is that RandomTrackNavigator needs
             * a list, and it would give bad performance to convert the whole playlist
             * from QSet to QList on every 'planOne()' call.
             */
            QList<quint64> allItemsList() { doItemListsMaintenance(); return m_allItemsList; }
            QSet<quint64> allItemsSet() { doItemListsMaintenance(); return m_allItemsList.toSet(); }

            /**
             * Items that we've played.
             */
            QList<quint64> historyItems() { doItemListsMaintenance(); return m_historyItems; }

        protected:
            /**
             * Load the contents of the source model.
             *
             * The bottom-most child class constructor should call this function. We must
             * not do it in our constructor or in intermediate constructors, because it
             * causes calls to the virtual functions below. And virtual functions don't do
             * what you want while you're in a base class constructor.
             */
            void loadFromSourceModel() { slotModelReset(); }

            /**
             * Request-callback for child classes: try to make sure that there's at least
             * 1 item in 'm_plannedItems'.
             */
            virtual void planOne() = 0;

            /**
             * Notification-callback for child classes: Items have been inserted.
             */
            virtual void notifyItemsInserted( const QSet<quint64> &insertedItems ) = 0;

            /**
             * Notification-callback for child classes: Items have been removed.
             * 'removedItems' may contain items that were not actually present.
             *
             * 'currentItem()' still has its old (possibly obsolete) value when this function is called.
             * This function can call 'setCurrentItem()' if it knows a good new choice.
             */
            virtual void notifyItemsRemoved( const QSet<quint64> &removedItems ) = 0;

            /**
             * List of planned-ahead playlist items. Should be edited by child classes.
             */
            QList<quint64> m_plannedItems;

        private slots:
            void slotModelReset();
            void slotRowsInserted( const QModelIndex& parent, int start, int end );
            void slotRowsRemoved( const QModelIndex& parent, int start, int end );

            void slotActiveTrackChanged( const quint64 );

        private:
            /**
             * Maintain internal tem lists. Call before doing anything with internal state.
             */
            void doItemListsMaintenance();

            /**
             * Choose the playlist item list that we'll take our next item from.
             */
            ItemList* nextItemChooseDonorList();


            /**
             * These variables hold inserted/removed items until we batch-process them.
             * This is due to performance requirements of slotRowsInserted/slotRowsRemoved.
             * Note: items may be in both
             */
            QSet<quint64> m_insertedItems;
            QSet<quint64> m_removedItems;


            /**
             * All items in the source model. See comments at 'allItemsList()'.
             */
            QList<quint64> m_allItemsList;

            /**
             * List of playlist items we already played. May contain items multiple times.
             */
            QList<quint64> m_historyItems;

            /**
             * The playlist item we most recently suggested to the rest of Amarok.
             */
            quint64 m_currentItem;

            /**
             * List of playlist items that had already made it to 'm_history', but have been
             * taken out again and re-played because the user did 'requestLastTrack()'.
             *
             * Makes 'requestNextTrack()' after 'requestLastTrack()' work predictably.
             *
             * We maintain this separately from 'm_plannedItems' because when new item are
             * inserted, they are mixed randomly with 'm_plannedItems'. This list shouldn't
             * change under those circumstances.
             */
            QList<quint64> m_replayedItems;
    };

}

#endif
