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

#ifndef APG_CHECKPOINT_CONSTRAINT
#define APG_CHECKPOINT_CONSTRAINT

#include "ui_CheckpointEditWidget.h"

#include "playlistgenerator/Constraint.h"

#include "core/meta/Meta.h"

#include <QString>

class ConstraintFactoryEntry;
class QWidget;

namespace Collections {
    class QueryMaker;
}

namespace ConstraintTypes {

    /* This constraint sets a "checkpoint": a specific track that will be
     * played at a specific time in the playlist */

    enum CheckpointType { CheckpointTrack, CheckpointAlbum, CheckpointArtist };

    class Checkpoint : public Constraint {
        Q_OBJECT

        public:
            static Constraint* createFromXml(QDomElement&, ConstraintNode*);
            static Constraint* createNew(ConstraintNode*);
            static ConstraintFactoryEntry* registerMe();

            virtual QWidget* editWidget() const;
            virtual void toXml(QDomDocument&, QDomElement&) const;

            virtual QString getName() const;

            virtual Collections::QueryMaker* initQueryMaker(Collections::QueryMaker*) const;
            virtual double satisfaction( const Meta::TrackList& );
            virtual double deltaS_insert( const Meta::TrackList&, const Meta::TrackPtr, const int ) const;
            virtual double deltaS_replace( const Meta::TrackList&, const Meta::TrackPtr, const int ) const;
            virtual double deltaS_delete( const Meta::TrackList&, const int ) const;
            virtual double deltaS_swap( const Meta::TrackList&, const int, const int ) const;
            virtual void insertTrack( const Meta::TrackList&, const Meta::TrackPtr, const int );
            virtual void replaceTrack( const Meta::TrackList&, const Meta::TrackPtr, const int );
            virtual void deleteTrack( const Meta::TrackList&, const int );
            virtual void swapTracks( const Meta::TrackList&, const int, const int );

            virtual int suggestInitialPlaylistSize() const;
            ConstraintNode::Vote* vote( const Meta::TrackList&, const Meta::TrackList& ) const;

            virtual void audit( const Meta::TrackList& ) const;

        private slots:
            void setPosition( const int );
            void setStrictness( const int );
            void setCheckpoint( const Meta::DataPtr& );

        private:
            /* support classes */
            class AbstractMatcher;
            class TrackMatcher;
            class ArtistMatcher;
            class AlbumMatcher;
            class BoundaryTracker;

            Checkpoint( QDomElement&, ConstraintNode* );
            Checkpoint( ConstraintNode* );
            ~Checkpoint();

            // constraint parameters
            qint64 m_position;
            double m_strictness;
            Meta::DataPtr m_checkpointObject;
            CheckpointType m_checkpointType;

            // helper functions
            qint64 findDistanceFor( const Meta::TrackList&, const BoundaryTracker* const ) const;
            double penalty( const qint64 ) const;

            // internal state variables
            qint64 m_distance; // how far a track is (in milliseconds) from its checkpoint

            // objects from the support classes
            AbstractMatcher* m_handler;
            BoundaryTracker* m_tracker;
    };

    // ABC for Checkpoint handlers
    class Checkpoint::AbstractMatcher {
        public:
            AbstractMatcher() {}
            virtual ~AbstractMatcher() {}

            virtual QList<int> find( const Meta::TrackList& ) const = 0;
            virtual bool match( const Meta::TrackPtr& ) const = 0;
            virtual Meta::TrackPtr suggest( const Meta::TrackList& ) const = 0;
    };

    // Handles Checkpoints for a specific track
    class Checkpoint::TrackMatcher : public AbstractMatcher {
        public:
            TrackMatcher( const Meta::TrackPtr& );
            virtual ~TrackMatcher() {}

            virtual QList<int> find( const Meta::TrackList& ) const;
            virtual bool match( const Meta::TrackPtr& ) const;
            virtual Meta::TrackPtr suggest( const Meta::TrackList& ) const;

        private:
            const Meta::TrackPtr m_trackToMatch;
    };

    // Handles Checkpoints for an artist (any track by this artist satisfies the checkpoint)
    class Checkpoint::ArtistMatcher : public AbstractMatcher {
        public:
            ArtistMatcher( const Meta::ArtistPtr& );
            virtual ~ArtistMatcher() {}

            virtual QList<int> find( const Meta::TrackList& ) const;
            virtual bool match( const Meta::TrackPtr& ) const;
            virtual Meta::TrackPtr suggest( const Meta::TrackList& ) const;

        private:
            const Meta::ArtistPtr m_artistToMatch;
    };

    // Handles Checkpoints for an album (any track on this album satisfies the checkpoint)
    class Checkpoint::AlbumMatcher : public AbstractMatcher {
        public:
            AlbumMatcher( const Meta::AlbumPtr& );
            virtual ~AlbumMatcher() {}

            virtual QList<int> find( const Meta::TrackList& ) const;
            virtual bool match( const Meta::TrackPtr& ) const;
            virtual Meta::TrackPtr suggest( const Meta::TrackList& ) const;

        private:
            const Meta::AlbumPtr m_albumToMatch;
    };

    class Checkpoint::BoundaryTracker {
        public:
            BoundaryTracker( const Meta::TrackList& );
            ~BoundaryTracker();
            QPair<qint64, qint64> getBoundariesAt( int ) const;
            int indexAtTime( qint64 ) const;

            void insertTrack( const Meta::TrackPtr, const int );
            void replaceTrack( const Meta::TrackPtr, const int );
            void deleteTrack( const int );
            void swapTracks( const int, const int );

            BoundaryTracker* cloneAndInsert( const Meta::TrackPtr, const int ) const;
            BoundaryTracker* cloneAndReplace( const Meta::TrackPtr, const int ) const;
            BoundaryTracker* cloneAndDelete( const int ) const;
            BoundaryTracker* cloneAndSwap( const int, const int ) const;

            void audit( const Meta::TrackList& ) const;

        private:
            BoundaryTracker( const QList<qint64>& );
            QList<qint64> m_endpoints;
    };

    class CheckpointEditWidget : public QWidget {
        Q_OBJECT

        public:
            CheckpointEditWidget( const qint64, const int, const Meta::DataPtr& );

        signals:
            void updated();
            void nameChanged( const QString& );
            void positionChanged( const int );
            void strictnessChanged( const int );
            void checkpointChanged( const Meta::DataPtr& );

        private slots:
            void on_timeEdit_Position_timeChanged( const QTime& );
            void on_slider_Strictness_valueChanged( const int );
            void on_trackSelector_selectionChanged( const Meta::DataPtr& );

        private:
            Ui::CheckpointEditWidget ui;
    };
} // namespace ConstraintTypes

#endif
