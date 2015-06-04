/****************************************************************************************
 * Copyright (c) 2008-2012 Soren Harward <stharward@gmail.com>                          *
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

#include "core/meta/forward_declarations.h"

#include <QPointer>
#include <QString>

class ConstraintFactoryEntry;
class QWidget;

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

            virtual double satisfaction( const Meta::TrackList& ) const;
            virtual quint32 suggestPlaylistSize() const;

        private Q_SLOTS:
            void setPosition( const int );
            void setStrictness( const int );
            void setCheckpoint( const Meta::DataPtr& );

        private:
            /* support classes */
            class AbstractMatcher;
            class TrackMatcher;
            class ArtistMatcher;
            class AlbumMatcher;

            Checkpoint( QDomElement&, ConstraintNode* );
            Checkpoint( ConstraintNode* );
            ~Checkpoint();

            // constraint parameters
            qint64 m_position;
            double m_strictness;
            Meta::DataPtr m_checkpointObject;
            CheckpointType m_checkpointType;

            // objects from the support classes
            QPointer<AbstractMatcher> m_matcher;

            // support functions
            double penalty( const qint64 ) const;
    };

    // ABC for Checkpoint handlers
    class Checkpoint::AbstractMatcher : public QObject {
        Q_OBJECT
        
        public:
            AbstractMatcher() {}
            virtual ~AbstractMatcher() {}

            virtual QList<int> find( const Meta::TrackList& ) const = 0;
            virtual bool match( const Meta::TrackPtr& ) const = 0;
    };

    // Handles Checkpoints for a specific track
    class Checkpoint::TrackMatcher : public AbstractMatcher {
        public:
            TrackMatcher( const Meta::TrackPtr& );
            virtual ~TrackMatcher();

            virtual QList<int> find( const Meta::TrackList& ) const;
            virtual bool match( const Meta::TrackPtr& ) const;

        private:
            Meta::TrackPtr m_trackToMatch;
    };

    // Handles Checkpoints for an album (any track on this album satisfies the checkpoint)
    class Checkpoint::AlbumMatcher : public AbstractMatcher {
        public:
            AlbumMatcher( const Meta::AlbumPtr& );
            virtual ~AlbumMatcher();

            virtual QList<int> find( const Meta::TrackList& ) const;
            virtual bool match( const Meta::TrackPtr& ) const;

        private:
            Meta::AlbumPtr m_albumToMatch;
    };

    // Handles Checkpoints for an artist (any track by this artist satisfies the checkpoint)
    class Checkpoint::ArtistMatcher : public AbstractMatcher {
        public:
            ArtistMatcher( const Meta::ArtistPtr& );
            virtual ~ArtistMatcher();

            virtual QList<int> find( const Meta::TrackList& ) const;
            virtual bool match( const Meta::TrackPtr& ) const;

        private:
            Meta::ArtistPtr m_artistToMatch;
    };

    class CheckpointEditWidget : public QWidget {
        Q_OBJECT

        public:
            CheckpointEditWidget( const qint64, const int, const Meta::DataPtr& );

        Q_SIGNALS:
            void updated();
            void nameChanged( const QString& );
            void positionChanged( const int );
            void strictnessChanged( const int );
            void checkpointChanged( const Meta::DataPtr& );

        private Q_SLOTS:
            void on_timeEdit_Position_timeChanged( const QTime& );
            void on_slider_Strictness_valueChanged( const int );
            void on_trackSelector_selectionChanged( const Meta::DataPtr& );

        private:
            Ui::CheckpointEditWidget ui;
    };
} // namespace ConstraintTypes

#endif
