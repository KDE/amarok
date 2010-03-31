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

    /* TODO: the internal math hasn't been written yet, so this constraint
     * doesn't do anything yet -- sth */

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
            virtual double satisfaction(const Meta::TrackList&);
            virtual double deltaS_insert(const Meta::TrackList&, const Meta::TrackPtr, const int) const;
            virtual double deltaS_replace(const Meta::TrackList&, const Meta::TrackPtr, const int) const;
            virtual double deltaS_delete(const Meta::TrackList&, const int) const;
            virtual double deltaS_swap(const Meta::TrackList&, const int, const int) const;
            virtual void insertTrack(const Meta::TrackList&, const Meta::TrackPtr, const int);
            virtual void replaceTrack(const Meta::TrackList&, const Meta::TrackPtr, const int);
            virtual void deleteTrack(const Meta::TrackList&, const int);
            virtual void swapTracks(const Meta::TrackList&, const int, const int);
            ConstraintNode::Vote* vote( const Meta::TrackList&, const Meta::TrackList& ) const;

        private slots:
            void setPosition( const int );
            void setStrictness( const int );
            void setCheckpoint( const Meta::DataPtr& );

        private:
            class AbstractCheckpointHandler; // abstract base class
            class TrackCheckpointHandler;
            class ArtistCheckpointHandler;
            class AlbumCheckpointHandler;

            Checkpoint(QDomElement&, ConstraintNode*);
            Checkpoint(ConstraintNode*);

            // constraint parameters
            int m_position;
            double m_strictness;
            Meta::DataPtr m_checkpointObject;
            CheckpointType m_checkpointType;

            double penalty( const int ) const;

            // internal state variables
            int m_distance; // how far a track is (in seconds) from its checkpoint

            // checkpoint handling strategy
            AbstractCheckpointHandler* m_handler;
    };

    // ABC for Checkpoint handlers
    class Checkpoint::AbstractCheckpointHandler {
        public:
            AbstractCheckpointHandler( const int );
            virtual ~AbstractCheckpointHandler() {}

            virtual int find( const Meta::TrackList& ) const = 0;
            virtual bool match( const Meta::TrackPtr& ) const = 0;
            virtual Meta::TrackPtr suggest( const Meta::TrackList& ) const = 0;

        protected:
            int m_checkpointPosition;
    };

    // Handles Checkpoints for a specific track
    class Checkpoint::TrackCheckpointHandler : public AbstractCheckpointHandler {
        public:
            TrackCheckpointHandler( const int, const Meta::TrackPtr& );
            virtual ~TrackCheckpointHandler() {}

            virtual int find( const Meta::TrackList& ) const;
            virtual bool match( const Meta::TrackPtr& ) const;
            virtual Meta::TrackPtr suggest( const Meta::TrackList& ) const;

        private:
            Meta::TrackPtr m_trackToMatch;
    };

    // Handles Checkpoints for an artist (any track by this artist satisfies the checkpoint)
    class Checkpoint::ArtistCheckpointHandler : public AbstractCheckpointHandler {
        public:
            ArtistCheckpointHandler( const int, const Meta::ArtistPtr& );
            virtual ~ArtistCheckpointHandler() {}

            virtual int find( const Meta::TrackList& ) const;
            virtual bool match( const Meta::TrackPtr& ) const;
            virtual Meta::TrackPtr suggest( const Meta::TrackList& ) const;

        private:
            Meta::ArtistPtr m_artistToMatch;
    };

    // Handles Checkpoints for an album (any track on this album satisfies the checkpoint)
    class Checkpoint::AlbumCheckpointHandler : public AbstractCheckpointHandler {
        public:
            AlbumCheckpointHandler( const int, const Meta::AlbumPtr& );
            virtual ~AlbumCheckpointHandler() {}

            virtual int find( const Meta::TrackList& ) const;
            virtual bool match( const Meta::TrackPtr& ) const;
            virtual Meta::TrackPtr suggest( const Meta::TrackList& ) const;

        private:
            Meta::AlbumPtr m_albumToMatch;
    };

    class CheckpointEditWidget : public QWidget {
        Q_OBJECT

        public:
            CheckpointEditWidget( const int, const int );

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
