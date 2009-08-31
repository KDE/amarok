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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef BOOKMARKPOPUP_H
#define BOOKMARKPOPUP_H

#include <QPaintEvent>
#include <QWidget>
class BookmarkPopup : public QWidget
{
public:
    BookmarkPopup ( QWidget* parent, QString label);

    virtual QSize sizeHint () const;
    virtual QSizePolicy sizePolicy() const;
    virtual QSize minimumSizeHint () const;

protected:

    virtual void paintEvent ( QPaintEvent* );

private:
    QString m_label;
    int m_width;
    int m_height;

    int m_lineHeight;
};

#endif // BOOKMARKPOPUP_H
