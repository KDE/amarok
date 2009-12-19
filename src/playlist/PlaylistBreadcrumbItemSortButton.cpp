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
    , m_noArrows( false )
    , m_arrowPressed( false )
    , m_arrowHovered( false )
{
    init();
}

BreadcrumbItemSortButton::BreadcrumbItemSortButton( const QIcon &icon, const QString &text, bool noArrows, QWidget *parent )
    : BreadcrumbItemButton( icon, text, parent )
    , m_order( Qt::AscendingOrder )
    , m_noArrows( noArrows )
    , m_arrowPressed( false )
{
    init();
}

BreadcrumbItemSortButton::~BreadcrumbItemSortButton()
{}

void
BreadcrumbItemSortButton::init()
{
    setMouseTracking( true );
    emit arrowToggled( m_order );
    repaint();
}

void
BreadcrumbItemSortButton::paintEvent( QPaintEvent *event )
{
    if( !m_noArrows )
    {
        QPainter painter(this);

        const int buttonHeight = height();
        int buttonWidth = width();
        int preferredWidth = sizeHint().width();
        if (preferredWidth < minimumWidth())
            preferredWidth = minimumWidth();
        if (buttonWidth > preferredWidth)
            buttonWidth = preferredWidth;

        int left, top, right, bottom;
        getContentsMargins ( &left, &top, &right, &bottom );
        const int padding = 2;

        const int arrowWidth = 10;
        const int arrowHeight = 10;
        const int arrowLeft = buttonWidth - arrowWidth - padding;
        const int arrowTop = ( ( buttonHeight - top - bottom) - arrowHeight )/2;
        m_arrowRect = QRect( arrowLeft, arrowTop, arrowWidth, arrowHeight );

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

        QRect newPaintRect( 0, 0, buttonWidth - arrowWidth - padding, buttonHeight );
        QPaintEvent *newEvent = new QPaintEvent( newPaintRect );
        BreadcrumbItemButton::paintEvent( newEvent );
    }
    else
        BreadcrumbItemButton::paintEvent( event );
}

void
BreadcrumbItemSortButton::drawHoverBackground( QPainter *painter )
{
    const bool isHovered = isDisplayHintEnabled( HoverHint );
    if( isHovered )
    {
        QColor backgroundColor = palette().color(QPalette::Highlight);
        QStyleOptionViewItemV4 option;
        option.initFrom(this);
        option.state = QStyle::State_Enabled | QStyle::State_MouseOver;
        option.viewItemPosition = QStyleOptionViewItemV4::OnlyOne;

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
    emit arrowToggled( m_order );
    repaint();
}

QSize
BreadcrumbItemSortButton::sizeHint() const
{
    if( !m_noArrows )
    {
        QSize size = BreadcrumbItemButton::sizeHint();
        size.setWidth( size.width() + 10 ); //+arrow width
        return size;
    }
    else
        return BreadcrumbItemButton::sizeHint();
}

Qt::SortOrder
BreadcrumbItemSortButton::orderState() const
{
    return m_order;
}

}
