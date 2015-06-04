/****************************************************************************************
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef AMAROK_COMBO_BOX_H
#define AMAROK_COMBO_BOX_H

#include <KComboBox> //baseclass

class QKeyEvent;

namespace Amarok
{

/**
 * The Amarok::ComboBox class implements a few enhancements to an editable KComboBox.
 * Namely:
 *   1. Pressing the escape key clears the edit text
 *   2. Pressing down emits the downPressed signal
 *   3. Pressing enter adds the current text to the completion popup
 */
class ComboBox : public KComboBox
{
    Q_OBJECT

    public:
        ComboBox( QWidget *parent = 0 );
        virtual ~ComboBox() {};

    protected:
        virtual void keyPressEvent( QKeyEvent *event );

    Q_SIGNALS:
        void downPressed();
};

} // namespace AMAROK

#endif // AMAROK_COMBO_BOX_H
