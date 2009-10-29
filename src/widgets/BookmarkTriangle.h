/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org                             *
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

#ifndef BOOKMARKTRIANGLE_H
#define BOOKMARKTRIANGLE_H

#include "BookmarkPopup.h"

#include <QEvent>
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
    virtual QSize sizeHint() const;
    virtual QSizePolicy sizePolicy() const;
    virtual QSize minimumSizeHint() const;

    virtual void mousePressEvent( QMouseEvent * event );
    virtual void mouseReleaseEvent( QMouseEvent * event );
    virtual void enterEvent( QEvent * event );
    virtual void leaveEvent( QEvent * event );

    virtual void paintEvent( QPaintEvent* );
    virtual void timerEvent( QTimerEvent * event );

    virtual void hidePopup();
    virtual int getTimeValue();

signals:
    void clicked( int );
    void focused( int );

private Q_SLOTS:
    void deleteBookmark();

private:
    int m_mseconds;
    QString m_name;
    BookmarkPopup* m_tooltip;
    int m_timerId;
};
#endif // BOOKMARKTRIANGLE_H
