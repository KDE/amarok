/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
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
     *
     * This whole thing is a hack in so many ways... But I am sure it will also be useful
     * elsewhere as it is really something I think that QPushButton should support by default
     * @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
     */
    class ElidingButton : public QPushButton
    {
        Q_OBJECT
    public:
        ElidingButton( QWidget *parent = 0 );
        ElidingButton( const QString & text, QWidget * parent = 0 );
        ElidingButton( const QIcon & icon, const QString & text, QWidget * parent = 0 );

        ~ElidingButton();

        QSizePolicy sizePolicy () const;

        virtual void resizeEvent ( QResizeEvent * event );

        //These 4 needs to be short circuited as _something, somewhere_ in the
        //Qt stack calls these at will and makes it impossible to change the
        //size of a QPushButton...
        void setFixedHeight ( int h );
        void setFixedSize ( const QSize & s );
        void setFixedSize ( int w, int h );
        void setFixedWidth ( int w );

    signals:
        void sizePolicyChanged();

    private:
        QString m_fullText;
        bool m_isElided;
    };
}

#endif
