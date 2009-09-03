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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "FramedLabel.h"

#include <QPainter>
#include <QPaintEvent>
#include <QStyleOption>

FramedLabel::FramedLabel( QWidget *parent, Qt::WindowFlags f )
        : QLabel( parent, f )
{}

FramedLabel::FramedLabel( const QString &text, QWidget *parent, Qt::WindowFlags f )
        : QLabel( text, parent, f )
{}

FramedLabel::~FramedLabel()
{}

void
FramedLabel::paintEvent( QPaintEvent *event )
{
    Q_UNUSED( event )
    if( frameShape() == QFrame::StyledPanel )
    {
        QPainter painter( this );
        QStyleOptionViewItemV4 option;
        option.initFrom( this );
        option.state = QStyle::State_Enabled | QStyle::State_MouseOver;
        option.viewItemPosition = QStyleOptionViewItemV4::OnlyOne;
        style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, &painter, this );
    }
    QRect cr = contentsRect();
    QPaintEvent *e = new QPaintEvent( cr );
    QLabel::paintEvent( e );
}
