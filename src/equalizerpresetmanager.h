/****************************************************************************************
 * Copyright (c) 2005 Markus Brueffer <markus@brueffer.de>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_EQUALIZERPRESETMANAGER_H
#define AMAROK_EQUALIZERPRESETMANAGER_H

#include "equalizersetup.h"

#include <KDialog>      //baseclass

class QPushButton;
class K3ListView;

class EqualizerPresetManager : public KDialog
{
    Q_OBJECT

    public:
        explicit EqualizerPresetManager( QWidget *parent = 0 );
        virtual ~EqualizerPresetManager();

        void setPresets(QMap< QString, QList<int> > presets);
        QMap< QString, QList<int> > presets();

    private slots:
        void slotRename();
        void slotDelete();
        void slotDefault();

        void updateButtonState();

    private:
        QMap< QString, QList<int> > m_presets;
        K3ListView* m_presetsView;

        //QPushButton* m_addBtn;
        QPushButton* m_renameBtn;
        QPushButton* m_deleteBtn;
};


#endif /* AMAROK_EQUALIZERPRESETMANAGER_H */
