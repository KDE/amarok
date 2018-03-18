/****************************************************************************************
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_LINE_EDIT_H
#define AMAROK_LINE_EDIT_H

#include <QLineEdit> //baseclass

class QKeyEvent;

namespace Amarok
{
    /**
     * The Amarok::LineEdit class implements a few enhancements to QLineEdit
     * Namely:
     *   1. Pressing the escape key clears the contents
     *   2. Pressing down emits the downPressed signal
     *   3. Pressing up emits the upPressed signal
     */
    class LineEdit : public QLineEdit
    {
        Q_OBJECT

        public:
            explicit LineEdit( QWidget *parent = 0 );
            virtual ~LineEdit() {};

            void keyPressEvent( QKeyEvent *event );

        Q_SIGNALS:
            void downPressed();
            void upPressed();
    };
}

#endif
