/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#ifndef ELIDINGBUTTON_H
#define ELIDINGBUTTON_H

#include <QPushButton>
#include <QResizeEvent>
#include <QSize>
#include <QSizePolicy>

namespace Amarok
{
    /**
     * This is a reimplementaiton of a QPushButton that elides text if stretched below
     * its optimal width. The icon (if any) will always remain visible.
     */
    class ElidingButton : public QPushButton
    {
        Q_OBJECT

        public:
            ElidingButton( QWidget *parent );
            ElidingButton( const QString & text, QWidget *parent );
            ElidingButton( const QIcon & icon, const QString & text, QWidget *parent );
            ~ElidingButton();

            bool isElided() const;
            QSizePolicy sizePolicy() const;

            virtual void setText( const QString &text );
            virtual void resizeEvent( QResizeEvent *event );

        signals:
            void sizePolicyChanged();

        private:
            void init();
            void elideText( const QSize &widgetSize );

            QString m_fullText;
            bool m_isElided;
    };
}

#endif
