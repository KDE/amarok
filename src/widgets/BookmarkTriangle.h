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
#include <QPoint>

class QSize;
class QSizePolicy;


class BookmarkTriangle : public QWidget
{
    Q_OBJECT
public:
    BookmarkTriangle( QWidget *parent, int milliseconds, QString name, int sliderwidth,
                      bool showPopup = false );
    ~BookmarkTriangle();
    virtual QSize sizeHint() const;
    virtual QSizePolicy sizePolicy() const;
    virtual QSize minimumSizeHint() const;

    virtual void showEvent ( QShowEvent * event );
    virtual void mousePressEvent ( QMouseEvent * event );
    virtual void mouseMoveEvent ( QMouseEvent * event );
    virtual void mouseReleaseEvent  (QMouseEvent *);
    virtual void enterEvent ( QEvent * event );
    virtual void leaveEvent ( QEvent * event );
    virtual void paintEvent ( QPaintEvent* );

    virtual void hidePopup();

    /**
     * Updates the position of the bookmark named @param name to @param newMiliseconds.
     *
     * The name should be a valid existing bookmark name and should include the trailing
     * "- m:ss"
     */
    virtual void moveBookmark( qint64 newMilliseconds, QString name );

    virtual void deleteBookmark();
    virtual int getTimeValue();

Q_SIGNALS:
    void clicked ( int );
    void focused ( int );

private:
    void initPopup();

    int m_mseconds; /// position of the bookmark on the slider in terms of milliseconds
    QString m_name; /// name of the bookmark
    int m_sliderwidth; /// width of the slider on which the bookmark will appear
    bool m_showPopup; /// used to determine whether to show the Pop-up on focussing the bookmark
    BookmarkPopup* m_tooltip; /// the tooltip that appears on focussing the bookmark
    QPoint m_offset; /// used while moving the bookmark, holds the position of the bookmark before moving
    int m_pos; /// used while moving the bookmark, holds the x co-ordinate of the bookmark after moving
};
#endif // BOOKMARKTRIANGLE_H
