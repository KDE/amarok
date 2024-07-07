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

#include <QIcon>
#include <QLineEdit>
#include <QPaintEvent>
#include <QTimer>
#include <QWidget>

class BookmarkTriangle;

class BookmarkPopup : public QWidget
{
    Q_OBJECT

public:
    explicit BookmarkPopup ( QWidget* parent, const QString &label, BookmarkTriangle* triangle );

    QSize sizeHint () const override;
    virtual QSizePolicy sizePolicy () const;
    QSize minimumSizeHint () const override;

    void mouseReleaseEvent ( QMouseEvent * event ) override;
    void mouseMoveEvent ( QMouseEvent * event ) override;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent ( QEvent* ) override;
#else
    void enterEvent ( QEnterEvent* ) override;
#endif
    void leaveEvent ( QEvent* ) override;

    virtual void displayNeeded ( bool value );

protected:

    void paintEvent ( QPaintEvent* event ) override;

protected Q_SLOTS:
    virtual void editValueChanged();
    virtual void hideTimerAction();

private:

    bool isOverDeleteIcon ( QPoint arg1 );
    bool isOverTitleLabel ( QPoint arg1 );

    void adjustWidth ();
    void startHideTimer ();

    QTimer *m_timer;
    QString m_label;
    QIcon m_deleteIcon;
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
