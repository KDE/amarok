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

#ifndef APG_NODUPES_CONSTRAINT
#define APG_NODUPES_CONSTRAINT

#include "ui_PreventDuplicatesEditWidget.h"

#include "playlistgenerator/Constraint.h"

#include <QString>

class ConstraintFactoryEntry;
class QWidget;

namespace ConstraintTypes {

    /* Prevents duplicate tracks, albums, or artists from ending up in playlist */

    enum DupeField { DupeTrack, DupeAlbum, DupeArtist };

    class PreventDuplicates : public Constraint {
        Q_OBJECT

        public:
            static Constraint* createFromXml( QDomElement&, ConstraintNode* );
            static Constraint* createNew( ConstraintNode* );
            static ConstraintFactoryEntry* registerMe();

            QWidget* editWidget() const override;
            void toXml( QDomDocument&, QDomElement& ) const override;

            QString getName() const override;
            
            double satisfaction( const Meta::TrackList& ) const override;

        private Q_SLOTS:
            void setField( const int );

        private:
            PreventDuplicates( QDomElement&, ConstraintNode* );
            explicit PreventDuplicates( ConstraintNode* );

            double penalty( const int ) const;

            // constraint parameters
            DupeField m_field;
    };

    class PreventDuplicatesEditWidget : public QWidget {
        Q_OBJECT

        public:
            explicit PreventDuplicatesEditWidget( const int );

        Q_SIGNALS:
            void updated();
            void fieldChanged( const int );

        private Q_SLOTS:
            void on_comboBox_Field_currentIndexChanged( const int );

        private:
            Ui::PreventDuplicatesEditWidget ui;
    };
} // namespace ConstraintTypes

#endif
