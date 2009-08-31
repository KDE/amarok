/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#ifndef AMAROK_FRAMEDLABEL_H
#define AMAROK_FRAMEDLABEL_H

#include <QLabel>

/**
 * A simple subclass of QLabel that unlike a QLabel under Oxygen, actually obeys setting
 * the shape to QFrame::StyledPanel.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class FramedLabel : public QLabel
{
    Q_OBJECT

public:
    explicit FramedLabel( QWidget *parent = 0, Qt::WindowFlags f = 0 );
    explicit FramedLabel( const QString &text, QWidget *parent = 0, Qt::WindowFlags f = 0 );
    ~FramedLabel();

protected:
    virtual void paintEvent( QPaintEvent *event );
};

#endif //AMAROK_FRAMEDLABEL_H
