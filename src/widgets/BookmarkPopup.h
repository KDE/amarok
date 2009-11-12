/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef BOOKMARKPOPUP_H
#define BOOKMARKPOPUP_H

#include <QLineEdit>
#include <QPaintEvent>
#include <QTimer>
#include <QWidget>

#include <KIcon>

class BookmarkTriangle;

class BookmarkPopup : public QWidget
{
    Q_OBJECT

public:
    BookmarkPopup ( QWidget* parent, QString label, BookmarkTriangle* triangle );

    virtual QSize sizeHint () const;
    virtual QSizePolicy sizePolicy () const;
    virtual QSize minimumSizeHint () const;

    virtual void mouseReleaseEvent ( QMouseEvent * event );
    virtual void mouseMoveEvent ( QMouseEvent * event );
    virtual void enterEvent ( QEvent* );
    virtual void leaveEvent ( QEvent* );

    virtual void displayNeeded ( bool value );

protected:

    virtual void paintEvent ( QPaintEvent* event );

protected slots:
    virtual void editValueChanged();
    virtual void hideTimerAction();

private:

    bool isOverDeleteIcon ( QPoint arg1 );
    bool isOverTitleLabel ( QPoint arg1 );

    void adjustWidth ();
    void startHideTimer ();

    QTimer *m_timer;
    QString m_label;
    KIcon m_deleteIcon;
    QRect m_deleteIconRect;
    QLineEdit *m_edit;
    int m_width;
    BookmarkTriangle *m_triangle;
    int m_height;
    int m_lineHeight;

    bool m_displayNeeded;
    bool m_hasMouseOver;
    bool m_overDelete;
    bool m_isEditMode;


};

#endif // BOOKMARKPOPUP_H
