/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef STATSYNCING_MATCHEDTRACKSPAGE_H
#define STATSYNCING_MATCHEDTRACKSPAGE_H

#include "ui_MatchedTracksPage.h"

namespace StatSyncing
{
    class MatchedTracksModel;
    class Provider;
    class SortFilterProxyModel;

    class MatchedTracksPage : public QWidget, private Ui::MatchedTracksPage
    {
        Q_OBJECT

        public:
            explicit MatchedTracksPage( QWidget *parent = 0, Qt::WindowFlags f = 0 );
            virtual ~MatchedTracksPage();

            /**
             * Set provider, you must call this before showing the widget.
             */
            void setProviders( const QList<QSharedPointer<Provider> > &providers );

            /**
             * Set mathed tracks model. MatchedTracksPage does _not_ take ownership of
             * the pointer.
             */
            void setMatchedTracksModel( MatchedTracksModel *model );

            /**
             * Add unique tracks model. MatchedTracksPage does _not_ take ownership of the
             * model pointer.
             */
            void addUniqueTracksModel( const Provider *provider, QAbstractItemModel *model );

            /**
             * Add excluded tracks model. MatchedTracksPage does _not_ take ownership of
             * the model pointer.
             */
            void addExcludedTracksModel( const Provider *provider, QAbstractItemModel *model );

        signals:
            /**
             * Emitted when the user clicks the Synchronize button. MatchedTracksPage
             * auto-destroys itself after emitting this signal.
             */
            void accepted();

            /**
             * Emitted when the user cancels or closes the dialog. MatchedTracksPage
             * auto-destroys itself after emitting this signal.
             */
            void rejected();

        protected:
            virtual void showEvent( QShowEvent *event );
            virtual void closeEvent( QCloseEvent *event );

        private slots:
            void showMatchedTracks( bool checked );
            void showUniqueTracks( bool checked );
            void showExcludedTracks( bool checked );
            /**
             * Helper method for show{Unique,Excluded}Tracks
             */
            void showSingleTracks( const QMap<const Provider *, QAbstractItemModel *> &models );

            void changeMatchedTracksFilter( int index );

            void changeUniqueTracksProvider( int index );
            void changeExcludedTracksProvider( int index );
            /**
             * Helper method for change{UniqueExcluded}TracksProvider
             */
            void changeSingleTracksProvider( int index, const QMap<const Provider *, QAbstractItemModel *> &models );

            void refreshStatusText();

            void rememberExpandedState( const QModelIndex &parent, int start, int end );
            void restoreExpandedState( const QModelIndex &parent, int start, int end );

            void takeRatingsFrom();

        private:
            void polish();
            void saveExpandedTuples();
            void restoreExpandedTuples();
            Q_DISABLE_COPY( MatchedTracksPage )

            bool m_polished;
            int m_matchedTracksComboLastIndex;
            QSet<int> m_expandedTuples;
            QList<QSharedPointer<Provider> > m_providers;
            SortFilterProxyModel *m_proxyModel;
            MatchedTracksModel *m_matchedTracksModel;
            QMap<const Provider *, QAbstractItemModel *> m_uniqueTracksModels;
            QMap<const Provider *, QAbstractItemModel *> m_excludedTracksModels;
    };

} // namespace StatSyncing

#endif // STATSYNCING_MATCHEDTRACKSPAGE_H
