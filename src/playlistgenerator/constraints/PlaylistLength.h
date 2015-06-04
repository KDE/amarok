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

#ifndef APG_PLAYLISTLENGTH_CONSTRAINT
#define APG_PLAYLISTLENGTH_CONSTRAINT

#include "ui_PlaylistLengthEditWidget.h"

#include "playlistgenerator/Constraint.h"

#include <QString>

class ConstraintFactoryEntry;
class QWidget;

namespace ConstraintTypes {

    /* This constraint derives its name from the fact that it specifies the
     * length (ie, number of tracks) of the Playlist. */

    class PlaylistLength : public Constraint {
        Q_OBJECT

        enum NumComparison { CompareNumLessThan, CompareNumEquals, CompareNumGreaterThan };

        public:
            static Constraint* createFromXml( QDomElement&, ConstraintNode* );
            static Constraint* createNew( ConstraintNode* );
            static ConstraintFactoryEntry* registerMe();

            virtual QWidget* editWidget() const;
            virtual void toXml( QDomDocument&, QDomElement& ) const;

            virtual QString getName() const;

            virtual double satisfaction( const Meta::TrackList& ) const;
            virtual quint32 suggestPlaylistSize() const;

        private Q_SLOTS:
            void setComparison( const int );
            void setLength( const int );
            void setStrictness( const int );

        private:
            PlaylistLength( QDomElement&, ConstraintNode* );
            PlaylistLength( ConstraintNode* );

            // constraint parameters
            quint32 m_length;
            int m_comparison;
            double m_strictness;

            // internal mathematical functions
            double transformLength( const int ) const;
    };

    class PlaylistLengthEditWidget : public QWidget {
        Q_OBJECT

        public:
            PlaylistLengthEditWidget( const int, const int, const int );

        Q_SIGNALS:
            void updated();
            void lengthChanged( const int );
            void comparisonChanged( const int );
            void strictnessChanged( const int );

        private Q_SLOTS:
            void on_spinBox_Length_valueChanged( const int );
            void on_comboBox_Comparison_currentIndexChanged( const int );
            void on_slider_Strictness_valueChanged( const int );

        private:
            Ui::PlaylistLengthEditWidget ui;
    };
} // namespace ConstraintTypes

#endif
