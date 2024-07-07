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
    BookmarkTriangle( QWidget *parent, int milliseconds, const QString &name, int sliderwidth,
                      bool showPopup = false );
    ~BookmarkTriangle() override;
    QSize sizeHint() const override;
    virtual QSizePolicy sizePolicy() const;
    QSize minimumSizeHint() const override;

    void showEvent ( QShowEvent * event ) override;
    void mousePressEvent ( QMouseEvent * event ) override;
    void mouseMoveEvent ( QMouseEvent * event ) override;
    void mouseReleaseEvent  (QMouseEvent *) override;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent( QEvent * ) override;
#else
    void enterEvent( QEnterEvent * ) override;
#endif
    void leaveEvent ( QEvent * event ) override;
    void paintEvent ( QPaintEvent* ) override;

    virtual void hidePopup();

    /**
     * Updates the position of the bookmark named @p name to @p newMilliseconds.
     *
     * The name should be a valid existing bookmark name and should include the trailing
     * "- m:ss"
     * @param name the name
     * @param newMilliseconds the new position in milliseconds
     */
    virtual void moveBookmark( qint64 newMilliseconds, const QString &name );

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
    bool m_showPopup; /// used to determine whether to show the Pop-up on focusing the bookmark
    BookmarkPopup* m_tooltip; /// the tooltip that appears on focusing the bookmark
    QPoint m_offset; /// used while moving the bookmark, holds the position of the bookmark before moving
    int m_pos; /// used while moving the bookmark, holds the x co-ordinate of the bookmark after moving
};
#endif // BOOKMARKTRIANGLE_H
