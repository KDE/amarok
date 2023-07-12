/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#include "PlaylistBreadcrumbItemSortButton.h"

#include <QIcon>

#include <QMargins>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>

namespace Playlist
{

BreadcrumbItemSortButton::BreadcrumbItemSortButton( QWidget *parent )
    : BreadcrumbItemButton( parent )
    , m_order( Qt::AscendingOrder )
    , m_arrowPressed( false )
    , m_arrowHovered( false )
    , m_arrowWidth( 11 )
    , m_arrowHeight( 13 )
{
    init();
}

BreadcrumbItemSortButton::BreadcrumbItemSortButton(const QIcon &icon, const QString &text, QWidget *parent )
    : BreadcrumbItemButton( icon, text, parent )
    , m_order( Qt::AscendingOrder )
    , m_arrowPressed( false )
    , m_arrowHovered( false )
    , m_arrowWidth( 11 )
    , m_arrowHeight( 13 )
{
    init();
}

BreadcrumbItemSortButton::~BreadcrumbItemSortButton()
{}

void
BreadcrumbItemSortButton::init()
{
    setMouseTracking( true );
    Q_EMIT arrowToggled( m_order );
    repaint();
}

void
BreadcrumbItemSortButton::paintEvent( QPaintEvent *event )
{
    Q_UNUSED( event )
    QPainter painter(this);

    const int buttonHeight = height();
    const int preferredWidth = qMax( minimumWidth(), sizeHint().width() );
    const int buttonWidth = qMin( preferredWidth, width() );

    QMargins margins = contentsMargins();
    const int padding = 2;

    const int arrowLeft = buttonWidth - m_arrowWidth - padding;
    const int arrowTop = ( ( buttonHeight - margins.top() - margins.bottom()) - m_arrowHeight )/2;
    m_arrowRect = QRect( arrowLeft, arrowTop, m_arrowWidth, m_arrowHeight );

    drawHoverBackground( &painter );

    const QColor fgColor = foregroundColor();
    QStyleOption option;
    option.initFrom(this);
    option.rect = m_arrowRect;
    option.palette = palette();
    option.palette.setColor(QPalette::Text, fgColor);
    option.palette.setColor(QPalette::WindowText, fgColor);
    option.palette.setColor(QPalette::ButtonText, fgColor);

    if( m_order == Qt::DescendingOrder )
        style()->drawPrimitive( QStyle::PE_IndicatorArrowDown, &option, &painter, this );
    else
        style()->drawPrimitive( QStyle::PE_IndicatorArrowUp, &option, &painter, this );

    QRect newPaintRect( 0, 0, buttonWidth - m_arrowWidth - padding, buttonHeight );
    QPaintEvent newEvent( newPaintRect );
    BreadcrumbItemButton::paintEvent( &newEvent );
}

void
BreadcrumbItemSortButton::drawHoverBackground( QPainter *painter )
{
    const bool isHovered = isDisplayHintEnabled( HoverHint );
    if( isHovered )
    {
        QStyleOptionViewItem option;
        option.initFrom(this);
        option.state = QStyle::State_Enabled | QStyle::State_MouseOver;
        option.viewItemPosition = QStyleOptionViewItem::OnlyOne;

        if( m_arrowHovered )
        {
            option.rect = m_arrowRect;
        }

        style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter, this );
    }
}

void
BreadcrumbItemSortButton::mouseMoveEvent( QMouseEvent *e )
{
    bool oldArrowHovered = m_arrowHovered;
    m_arrowHovered = m_arrowRect.contains( e->pos() );
    if( oldArrowHovered != m_arrowHovered )
        repaint();
    BreadcrumbItemButton::mouseMoveEvent( e );
}

void
BreadcrumbItemSortButton::mousePressEvent( QMouseEvent *e )
{
    m_pressedPos = e->pos();
    if( m_arrowRect.contains( m_pressedPos ) )
        m_arrowPressed = true;
    else
    {
        m_arrowPressed = false;
        BreadcrumbItemButton::mousePressEvent( e );
    }
}

void
BreadcrumbItemSortButton::mouseReleaseEvent( QMouseEvent *e )
{
    if( m_arrowPressed && e->pos() == m_pressedPos )
        invertOrder();
    BreadcrumbItemButton::mouseReleaseEvent( e );
}

void
BreadcrumbItemSortButton::invertOrder()
{
    if( m_order == Qt::DescendingOrder )
        m_order = Qt::AscendingOrder;
    else    //ascending
        m_order = Qt::DescendingOrder;
    Q_EMIT arrowToggled( m_order );
    repaint();
}

QSize
BreadcrumbItemSortButton::sizeHint() const
{
    QSize size = BreadcrumbItemButton::sizeHint();
    size.setWidth( size.width() + m_arrowWidth );
    return size;
}

Qt::SortOrder
BreadcrumbItemSortButton::orderState() const
{
    return m_order;
}

}
