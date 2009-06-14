/*****************************************************************************
 * Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                      *
 * Copyright (C) 2006 by Aaron J. Seigo <aseigo@kde.org>                     *
 * Copyright (C) 2009 by Seb Ruiz <ruiz@kde.org>                             *
 *                                                                           *
 * This library is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Library General Public               *
 * License version 2 as published by the Free Software Foundation.           *
 *                                                                           *
 * This library is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Library General Public License for more details.                          *
 *                                                                           *
 * You should have received a copy of the GNU Library General Public License *
 * along with this library; see the file COPYING.LIB.  If not, write to      *
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 * Boston, MA 02110-1301, USA.                                               *
 *****************************************************************************/

#include "BreadcrumbItemButton.h"

#include <KColorScheme>
#include <KIcon>
#include <KLocale>
#include <KMenu>

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionFocusRect>

BreadcrumbItemButton::BreadcrumbItemButton( QWidget *parent )
    : Amarok::ElidingButton( parent )
{
    init( parent );
}

BreadcrumbItemButton::BreadcrumbItemButton( const QIcon &icon, const QString &text, QWidget *parent )
    : Amarok::ElidingButton( icon, text, parent )
{
    init( parent );
}

void BreadcrumbItemButton::init( QWidget *parent )
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    setMinimumHeight(parent->minimumHeight());
}

BreadcrumbItemButton::~BreadcrumbItemButton()
{
}

void BreadcrumbItemButton::setDisplayHintEnabled(DisplayHint hint, bool enable)
{
    if (enable) {
        m_displayHint = m_displayHint | hint;
    } else {
        m_displayHint = m_displayHint & ~hint;
    }
    update();
}

bool BreadcrumbItemButton::isDisplayHintEnabled(DisplayHint hint) const
{
    return (m_displayHint & hint) > 0;
}

void BreadcrumbItemButton::enterEvent(QEvent* event)
{
    QPushButton::enterEvent(event);
    setDisplayHintEnabled(HoverHint, true);
    update();
}

void BreadcrumbItemButton::leaveEvent(QEvent* event)
{
    QPushButton::leaveEvent(event);
    setDisplayHintEnabled(HoverHint, false);
    update();
}

void BreadcrumbItemButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

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

    const QColor fgColor = foregroundColor();
    drawHoverBackground(&painter);

    int left, top, right, bottom;
    getContentsMargins ( &left, &top, &right, &bottom );
    const int padding = 2;

    const int iconWidth = iconSize().width();
    const int iconHeight = iconSize().height();
    const int iconTop = ( (buttonHeight - top - bottom) - iconHeight ) / 2;
    const QRect iconRect( left + padding, iconTop, iconWidth, iconHeight );
    painter.drawPixmap( iconRect, icon().pixmap( iconSize() ) );

    if( isDisplayHintEnabled(ActiveHint) )
    {
        QFont f = painter.font();
        f.setBold( true );
        painter.setFont( f );
    }

    painter.setPen(fgColor);
    const QRect textRect( left + (padding * 2) + iconWidth, top, buttonWidth, buttonHeight);
    painter.drawText(textRect, Qt::AlignVCenter, text());
}


void BreadcrumbItemButton::drawHoverBackground(QPainter* painter)
{
    const bool isHovered = isDisplayHintEnabled( HoverHint );

    if( isHovered )
    {
        QColor backgroundColor = palette().color(QPalette::Highlight);
        // TODO: the backgroundColor should be applied to the style
        QStyleOptionViewItemV4 option;
        option.initFrom(this);
        option.state = QStyle::State_Enabled | QStyle::State_MouseOver;
        option.viewItemPosition = QStyleOptionViewItemV4::OnlyOne;
        style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, this);
    }
}

QColor BreadcrumbItemButton::foregroundColor() const
{
    const bool isHighlighted = isDisplayHintEnabled( HoverHint );
    const bool isActive = isDisplayHintEnabled( ActiveHint );

    QColor foregroundColor = palette().color( foregroundRole() );
    if( !isActive && !isHighlighted )
        foregroundColor.setAlpha( 60 );

    return foregroundColor;
}

QSize BreadcrumbItemMenuButton::sizeHint() const
{
    QSize size = BreadcrumbItemButton::sizeHint();
    size.setWidth(size.height() / 2);
    return size;
}

void BreadcrumbItemMenuButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    drawHoverBackground(&painter);

    const QColor fgColor = foregroundColor();
    
    QStyleOption option;
    option.initFrom(this);
    option.rect = QRect(0, 0, width(), height());
    option.palette = palette();
    option.palette.setColor(QPalette::Text, fgColor);
    option.palette.setColor(QPalette::WindowText, fgColor);
    option.palette.setColor(QPalette::ButtonText, fgColor);
    
    if (layoutDirection() == Qt::LeftToRight) {
        style()->drawPrimitive(QStyle::PE_IndicatorArrowRight, &option, &painter, this);
    } else {
        style()->drawPrimitive(QStyle::PE_IndicatorArrowLeft, &option, &painter, this);
    }
}


#include "BreadcrumbItemButton.moc"
