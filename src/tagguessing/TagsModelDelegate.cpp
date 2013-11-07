/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 * Copyright (c) 2013 Alberto Villa <avilla@FreeBSD.org>                                *
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

#define DEBUG_PREFIX "TagsModelDelegate"

#include "TagsModelDelegate.h"

#include <QApplication>

using namespace TagGuessing;

TagsModelDelegate::TagsModelDelegate( QObject *parent )
    : QItemDelegate( parent )
{
}

void
TagsModelDelegate::drawCheck( QPainter *painter,
                                         const QStyleOptionViewItem &option,
                                         const QRect &rect, Qt::CheckState state ) const
{
    if( !rect.isValid() )
        return;

    QStyleOptionViewItem opt( option );
    opt.rect = rect;
    opt.state &= ~QStyle::State_HasFocus;

    switch( state )
    {
    case Qt::Unchecked:
        opt.state |= QStyle::State_Off;
        break;
    case Qt::PartiallyChecked:
        opt.state |= QStyle::State_NoChange;
        break;
    case Qt::Checked:
        opt.state |= QStyle::State_On;
        break;
    }

    QApplication::style()->drawPrimitive( QStyle::PE_IndicatorRadioButton, &opt, painter );
}
