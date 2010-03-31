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

#ifndef APG_PLAYLISTLENGTH_CONSTRAINT
#define APG_PLAYLISTLENGTH_CONSTRAINT

#include "ui_PlaylistLengthEditWidget.h"

#include "playlistgenerator/Constraint.h"

#include <QString>

class ConstraintFactoryEntry;
class QWidget;

namespace Collections {
    class QueryMaker;
}

namespace ConstraintTypes {

    /* This constraint derives its name from the fact that it specifies the
     * Length of the Playlist. */

    class PlaylistLength : public Constraint {
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
            virtual int suggestInitialPlaylistSize() const;
            ConstraintNode::Vote* vote( const Meta::TrackList&, const Meta::TrackList& ) const;

        private slots:
            void setComparison( const int );
            void setLength( const int );
            void setStrictness( const int );

        private:
            PlaylistLength(QDomElement&, ConstraintNode*);
            PlaylistLength(ConstraintNode*);

            // constraint parameters
            qint64 m_length; // time in msec
            int m_comparison;
            double m_strictness;

            // convenience functions
            QString comparisonToString() const;

            // internal mathematical functions
            double transformLength( const qint64 ) const;

            // internal mathematical state data
            qint64 m_totalLength;
    };

    class PlaylistLengthEditWidget : public QWidget {
        Q_OBJECT

        public:
            PlaylistLengthEditWidget( const int, const int, const int );

        signals:
            void updated();
            void lengthChanged( const int );
            void comparisonChanged( const int );
            void strictnessChanged( const int );

        private slots:
            void on_timeEdit_Duration_timeChanged( const QTime& );
            void on_comboBox_Comparison_currentIndexChanged( const int );
            void on_slider_Strictness_valueChanged( const int );

        private:
            Ui::PlaylistLengthEditWidget ui;
    };
} // namespace ConstraintTypes

#endif
