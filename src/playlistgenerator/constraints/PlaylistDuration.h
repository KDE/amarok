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

#ifndef APG_PLAYLISTDURATION_CONSTRAINT
#define APG_PLAYLISTDURATION_CONSTRAINT

#include "ui_PlaylistDurationEditWidget.h"

#include "playlistgenerator/Constraint.h"

#include <QString>

class ConstraintFactoryEntry;
class QWidget;

namespace ConstraintTypes {

    /* This constraint derives its name from the fact that it specifies the
     * duration (ie, total running time) of the Playlist. */

    class PlaylistDuration : public Constraint {
        Q_OBJECT

        enum NumComparison { CompareNumLessThan, CompareNumEquals, CompareNumGreaterThan };

        public:
            static Constraint* createFromXml(QDomElement&, ConstraintNode*);
            static Constraint* createNew(ConstraintNode*);
            static ConstraintFactoryEntry* registerMe();

            QWidget* editWidget() const override;
            void toXml(QDomDocument&, QDomElement&) const override;

            QString getName() const override;

            double satisfaction(const Meta::TrackList&) const override;
            quint32 suggestPlaylistSize() const override;

        private Q_SLOTS:
            void setComparison( const int );
            void setDuration( const int );
            void setStrictness( const int );

        private:
            PlaylistDuration(QDomElement&, ConstraintNode*);
            explicit PlaylistDuration(ConstraintNode*);

            // constraint parameters
            qint64 m_duration; // time in msec
            int m_comparison;
            double m_strictness;
    };

    class PlaylistDurationEditWidget : public QWidget {
        Q_OBJECT

        public:
            PlaylistDurationEditWidget( const int, const int, const int );

        Q_SIGNALS:
            void updated();
            void durationChanged( const int );
            void comparisonChanged( const int );
            void strictnessChanged( const int );

        private Q_SLOTS:
            void on_timeEdit_Duration_timeChanged( const QTime& );
            void on_comboBox_Comparison_currentIndexChanged( const int );
            void on_slider_Strictness_valueChanged( const int );

        private:
            Ui::PlaylistDurationEditWidget ui;
    };
} // namespace ConstraintTypes

#endif
