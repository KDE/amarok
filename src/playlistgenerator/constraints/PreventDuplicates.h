/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#ifndef APG_NODUPES_CONSTRAINT
#define APG_NODUPES_CONSTRAINT

#include "ui_PreventDuplicatesEditWidget.h"

#include "playlistgenerator/Constraint.h"

#include <QHash>
#include <QString>
#include <QVariant>

class ConstraintFactoryEntry;
class QWidget;

namespace Collections {
    class QueryMaker;
}

namespace ConstraintTypes {

    /* Prevents duplicate tracks, albums, or artists from ending up in playlist */

    enum DupeField { DupeTrack, DupeAlbum, DupeArtist };

    class PreventDuplicates : public Constraint {
        Q_OBJECT

        public:
            static Constraint* createFromXml( QDomElement&, ConstraintNode* );
            static Constraint* createNew( ConstraintNode* );
            static ConstraintFactoryEntry* registerMe();

            virtual QWidget* editWidget() const;
            virtual void toXml( QDomDocument&, QDomElement& ) const;

            virtual QString getName() const;
            
            virtual Collections::QueryMaker* initQueryMaker( Collections::QueryMaker* ) const;
            virtual double satisfaction( const Meta::TrackList& );
            virtual double deltaS_insert( const Meta::TrackList&, const Meta::TrackPtr, const int ) const;
            virtual double deltaS_replace( const Meta::TrackList&, const Meta::TrackPtr, const int ) const;
            virtual double deltaS_delete( const Meta::TrackList&, const int ) const;
            virtual double deltaS_swap( const Meta::TrackList&, const int, const int ) const;
            virtual void insertTrack( const Meta::TrackList&, const Meta::TrackPtr, const int );
            virtual void replaceTrack( const Meta::TrackList&, const Meta::TrackPtr, const int );
            virtual void deleteTrack( const Meta::TrackList&, const int );
            virtual void swapTracks( const Meta::TrackList&, const int, const int );

            ConstraintNode::Vote* vote( const Meta::TrackList&, const Meta::TrackList& ) const;

#ifndef KDE_NO_DEBUG_OUTPUT
            virtual void audit(const Meta::TrackList&) const;
#endif

        private slots:
            void setField( const int );

        private:
            // duplicate counting strategy declarations
            class DuplicateCounter; // abstract base class
            class TrackDuplicateCounter;
            class AlbumDuplicateCounter;
            class ArtistDuplicateCounter;

            PreventDuplicates( QDomElement&, ConstraintNode* );
            PreventDuplicates( ConstraintNode* );

            double penalty( const int ) const;

            // constraint parameters
            DupeField m_field;

            // counter strategy: changed according to m_field setting
            DuplicateCounter* m_counterPtr;
            int m_dupeCount;
    };

    // ABC for duplicate counting strategies
    class PreventDuplicates::DuplicateCounter {
        public:
            DuplicateCounter();
            virtual ~DuplicateCounter() {}

            int count() const { return m_duplicateCount; }
            virtual int insertionDelta( const Meta::TrackPtr ) const = 0;
            virtual int deletionDelta( const Meta::TrackPtr ) const = 0;
            virtual void insertTrack( const Meta::TrackPtr ) = 0;
            virtual void deleteTrack( const Meta::TrackPtr ) = 0;
            virtual int trackCount( const Meta::TrackPtr ) const = 0;

        protected:
            int m_duplicateCount;
    };

    // Counts the number of duplicate tracks in the playlist
    class PreventDuplicates::TrackDuplicateCounter : public PreventDuplicates::DuplicateCounter {
        public:
            TrackDuplicateCounter( const Meta::TrackList& );
            virtual ~TrackDuplicateCounter() {}

            virtual int insertionDelta( const Meta::TrackPtr ) const;
            virtual int deletionDelta( const Meta::TrackPtr ) const;
            virtual void insertTrack( const Meta::TrackPtr );
            virtual void deleteTrack( const Meta::TrackPtr );
            virtual int trackCount( const Meta::TrackPtr ) const;

        private:
            QHash<Meta::TrackPtr, int> m_trackCounts;
    };

    // Counts the number of duplicate albums in the playlist
    class PreventDuplicates::AlbumDuplicateCounter : public PreventDuplicates::DuplicateCounter {
        public:
            AlbumDuplicateCounter( const Meta::TrackList& );
            virtual ~AlbumDuplicateCounter() {}

            virtual int insertionDelta( const Meta::TrackPtr ) const;
            virtual int deletionDelta( const Meta::TrackPtr ) const;
            virtual void insertTrack( const Meta::TrackPtr );
            virtual void deleteTrack( const Meta::TrackPtr );
            virtual int trackCount( const Meta::TrackPtr ) const;

        private:
            QHash<Meta::AlbumPtr, int> m_albumCounts;
    };

    // Counts the number of duplicate artists in the playlist
    class PreventDuplicates::ArtistDuplicateCounter : public PreventDuplicates::DuplicateCounter {
        public:
            ArtistDuplicateCounter( const Meta::TrackList& );
            virtual ~ArtistDuplicateCounter() {}

            virtual int insertionDelta( const Meta::TrackPtr ) const;
            virtual int deletionDelta( const Meta::TrackPtr ) const;
            virtual void insertTrack( const Meta::TrackPtr );
            virtual void deleteTrack( const Meta::TrackPtr );
            virtual int trackCount( const Meta::TrackPtr ) const;

        private:
            QHash<Meta::ArtistPtr, int> m_artistCounts;
    };

    class PreventDuplicatesEditWidget : public QWidget {
        Q_OBJECT

        public:
            PreventDuplicatesEditWidget( const int );

        signals:
            void updated();
            void fieldChanged( const int );

        private slots:
            void on_comboBox_Field_currentIndexChanged( const int );

        private:
            Ui::PreventDuplicatesEditWidget ui;
    };
} // namespace ConstraintTypes

#endif
