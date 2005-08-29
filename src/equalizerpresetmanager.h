/***************************************************************************
 *   Copyright (C) 2005 by Markus Brueffer <markus@brueffer.de>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef AMAROK_EQUALIZERPRESETMANAGER_H
#define AMAROK_EQUALIZERPRESETMANAGER_H

#include "equalizersetup.h"

#include <kdialogbase.h>      //baseclass

class QPushButton;
class QStringList;
class KListView;

class EqualizerPresetManager : public KDialogBase
{
    Q_OBJECT

    public:
        EqualizerPresetManager( QWidget *parent = 0, const char *name = 0 );
        virtual ~EqualizerPresetManager();

        void setPresets(QMap< QString, QValueList<int> > presets);
        QMap< QString, QValueList<int> > presets();

    private slots:
        void slotRename();
        void slotDelete();
        void slotDefault();

        void updateButtonState();

    private:
        QMap< QString, QValueList<int> > m_presets;
        KListView* m_presetsView;

        //QPushButton* m_addBtn;
        QPushButton* m_renameBtn;
        QPushButton* m_deleteBtn;
};


#endif /* AMAROK_EQUALIZERPRESETMANAGER_H */
