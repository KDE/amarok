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
#include "statsyncing/Provider.h"

namespace StatSyncing
{
    class MatchedTracksModel;
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
            void setProviders( const ProviderPtrList &providers );

            /**
             * Set mathed tracks model. MatchedTracksPage does _not_ take ownership of
             * the pointer.
             */
            void setMatchedTracksModel( MatchedTracksModel *model );

            /**
             * Add unique tracks model. MatchedTracksPage does _not_ take ownership of the
             * model pointer.
             */
            void addUniqueTracksModel( ProviderPtr provider, QAbstractItemModel *model );

            /**
             * Add excluded tracks model. MatchedTracksPage does _not_ take ownership of
             * the model pointer.
             */
            void addExcludedTracksModel( ProviderPtr provider, QAbstractItemModel *model );

        signals:
            /**
             * Emitted when user pushes the Back button.
             */
            void back();

            /**
             * Emitted when user clicks the Synchronize button.
             */
            void accepted();

            /**
             * Emitted when user pushes the Cancel button.
             */
            void rejected();

        protected:
            virtual void showEvent( QShowEvent *event );

        private slots:
            void showMatchedTracks( bool checked );
            void showUniqueTracks( bool checked );
            void showExcludedTracks( bool checked );
            /**
             * Helper method for show{Unique,Excluded}Tracks
             */
            void showSingleTracks( const QMap<ProviderPtr, QAbstractItemModel *> &models );

            void changeMatchedTracksFilter( int index );

            void changeUniqueTracksProvider( int index );
            void changeExcludedTracksProvider( int index );
            /**
             * Helper method for change{UniqueExcluded}TracksProvider
             */
            void changeSingleTracksProvider( int index, const QMap<ProviderPtr, QAbstractItemModel *> &models );

            void refreshStatusText();

            void rememberExpandedState( const QModelIndex &parent, int start, int end );
            void restoreExpandedState( const QModelIndex &parent, int start, int end );

            void takeRatingsFrom();
            void includeLabelsFrom();
            void excludeLabelsFrom();

            void openConfiguration();

        private:
            void polish();
            void saveExpandedTuples();
            void restoreExpandedTuples();
            Q_DISABLE_COPY( MatchedTracksPage )

            bool m_polished;
            int m_matchedTracksComboLastIndex;
            QSet<int> m_expandedTuples;
            ProviderPtrList m_providers;
            SortFilterProxyModel *m_proxyModel;
            MatchedTracksModel *m_matchedTracksModel;
            QMap<ProviderPtr, QAbstractItemModel *> m_uniqueTracksModels;
            QMap<ProviderPtr, QAbstractItemModel *> m_excludedTracksModels;
    };

} // namespace StatSyncing

#endif // STATSYNCING_MATCHEDTRACKSPAGE_H
