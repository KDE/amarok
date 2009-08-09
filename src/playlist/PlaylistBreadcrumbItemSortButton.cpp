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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistBreadcrumbItemSortButton.h"

#include <KIcon>

#include <QPainter>
#include <QStyle>
#include <QStyleOption>

namespace Playlist
{

BreadcrumbItemSortButton::BreadcrumbItemSortButton( QWidget *parent )
    : BreadcrumbItemButton( parent )
    , m_order( Qt::AscendingOrder )
{
    init();
}

BreadcrumbItemSortButton::BreadcrumbItemSortButton( const QIcon &icon, const QString &text, QWidget *parent )
    : BreadcrumbItemButton( icon, text, parent )
    , m_order( Qt::AscendingOrder )
{
    init();
}

BreadcrumbItemSortButton::~BreadcrumbItemSortButton()
{}

void
BreadcrumbItemSortButton::init()
{

}

void
BreadcrumbItemSortButton::paintEvent( QPaintEvent *event )
{
    Q_UNUSED( event );
    QPainter painter(this);

    const int buttonHeight = height();
    int buttonWidth = width();
    int preferredWidth = sizeHint().width();
    if (preferredWidth < minimumWidth()) {
        preferredWidth = minimumWidth();
    }
    if (buttonWidth > preferredWidth) {
        buttonWidth = preferredWidth;
    }

    drawHoverBackground(&painter);

    int left, top, right, bottom;
    getContentsMargins ( &left, &top, &right, &bottom );
    const int padding = 2;

    const QColor fgColor = foregroundColor();

    const int arrowWidth = 10;
    const int arrowHeight = 10;
    const int arrowTop = ( buttonHeight - top - bottom) /2;
    const QRect arrowRect( buttonWidth - arrowWidth - padding, arrowTop, arrowWidth, arrowHeight );
    QStyleOption option;
    option.initFrom(this);
    option.rect = arrowRect;
    option.palette = palette();
    option.palette.setColor(QPalette::Text, fgColor);
    option.palette.setColor(QPalette::WindowText, fgColor);
    option.palette.setColor(QPalette::ButtonText, fgColor);

    style()->drawPrimitive( QStyle::PE_IndicatorArrowDown, &option, &painter, this );

    QRect newPaintRect( 0, 0, buttonWidth - arrowWidth - padding, buttonHeight );
    QPaintEvent *newEvent = new QPaintEvent( newPaintRect );
    BreadcrumbItemButton::paintEvent( newEvent );
}

QSize
BreadcrumbItemSortButton::sizeHint() const
{
    QSize size = BreadcrumbItemButton::sizeHint();
    size.setWidth( size.width() + 10 );
    return size;
}

Qt::SortOrder
BreadcrumbItemSortButton::orderState() const
{
    return m_order;
}

}
