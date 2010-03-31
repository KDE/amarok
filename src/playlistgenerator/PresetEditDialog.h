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

#ifndef APG_PRESET_EDIT_DIALOG
#define APG_PRESET_EDIT_DIALOG

#include "ui_PresetEditDialog.h"

#include "Preset.h"

#include <QDialog>
#include <QHash>

class ConstraintNode;

namespace APG {
    class TreeController;

	class PresetEditDialog : public QDialog {
        Q_OBJECT

        public:
            PresetEditDialog( PresetPtr );

        private slots:
            void addNode( const QString& );
            void removeNode();
            void currentNodeChanged( const QModelIndex& );

            void accept();
            void reject();

            void on_lineEdit_Title_textChanged( const QString& );

        private:
            Ui::PresetEditDialog ui;

            TreeController* m_controller;
            PresetPtr m_preset;

            QHash<ConstraintNode*, int> m_widgetStackPages;
    };
}

#endif
