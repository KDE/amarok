/***************************************************************************
*   Copyright (c) 2009  Casey Link <unnamedrambler@gmail.com>             *
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

#ifndef BOOKMARKTRIANGLE_H
#define BOOKMARKTRIANGLE_H

#include "Meta.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QWidget>

class QSize;
class QSizePolicy;


class BookmarkTriangle : public QWidget
{
    Q_OBJECT
public:
    BookmarkTriangle ( QWidget *parent, int milliseconds, QString name );
    ~BookmarkTriangle();
    virtual QSize sizeHint () const;
    virtual QSizePolicy sizePolicy() const;
    virtual QSize minimumSizeHint () const;

    virtual void mousePressEvent ( QMouseEvent * event );
    virtual void mouseReleaseEvent ( QMouseEvent * event );

    virtual void paintEvent ( QPaintEvent* );

signals:
    void clicked( int );

private:
    int m_mseconds;
    QString m_name;
};

#endif // BOOKMARKTRIANGLE_H
