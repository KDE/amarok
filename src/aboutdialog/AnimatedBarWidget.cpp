/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2012 Lachlan Dufton <dufton@gmail.com>                                 *
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

#include "AnimatedBarWidget.h"

#include <QIcon>

#include <QHBoxLayout>
#include <QMargins>
#include <QPainter>
#include <QStyleOption>

AnimatedBarWidget::AnimatedBarWidget( const QIcon &icon, const QString &text, const QString &animatedIconName, QWidget *parent )
    : QAbstractButton( parent )
{
    setIconSize( QSize( 22, 22 ) );
    setIcon( icon );
    setText( text );
    m_animating = false;
    m_animatedWidget = new AnimatedWidget( animatedIconName, this );
    m_animatedWidget->setFixedSize( 22, 22 );
    m_animatedWidget->hide();
    m_animatedWidget->setAutoFillBackground( false );

    setFocusPolicy( Qt::NoFocus );
    m_hoverHint = false;

    // Need to change the size policy to allow for wrapping
    QSizePolicy poli( QSizePolicy::Preferred, QSizePolicy::Preferred );
    poli.setHeightForWidth( true );
    setSizePolicy( poli );
}

AnimatedBarWidget::~AnimatedBarWidget()
{}

void
AnimatedBarWidget::animate()
{
    m_animating = true;
    m_animatedWidget->show();
    m_animatedWidget->start();
    update();
}

void
AnimatedBarWidget::stop()
{
    m_animating = false;
    m_animatedWidget->stop();
    m_animatedWidget->hide();
    update();
}

void
AnimatedBarWidget::fold()
{
    hide();
}

//protected:



void
AnimatedBarWidget::setHoverHintEnabled( bool enable )
{
    m_hoverHint = enable;
    update();
}

bool
AnimatedBarWidget::isHoverHintEnabled() const
{
    return m_hoverHint;
}

void
AnimatedBarWidget::enterEvent( QEvent* event )
{
    QWidget::enterEvent( event );
    setHoverHintEnabled( true );
    update();
}

void
AnimatedBarWidget::leaveEvent( QEvent* event )
{
    QWidget::leaveEvent( event );
    setHoverHintEnabled( false );
    update();
}

void
AnimatedBarWidget::paintEvent( QPaintEvent* event )
{
    Q_UNUSED(event);

    QPainter painter(this);

    const int buttonHeight = height();
    int buttonWidth = width();

    drawHoverBackground(&painter);

    QMargins margins = contentsMargins();
    const int padding = 2;
    const int iconWidth = iconSize().width();
    const int iconHeight = iconSize().height();
    const int iconTop = ( (buttonHeight - margins.top() - margins.bottom()) - iconHeight ) / 2;

    if( !m_animating )
    {
        const QRect iconRect( margins.left() + padding, iconTop, iconWidth, iconHeight );
        painter.drawPixmap( iconRect, icon().pixmap( iconSize() ) );
    }
    else
        m_animatedWidget->move( margins.left() + padding, iconTop );

    const QRect textRect( margins.left() + (padding * 3) + iconWidth, margins.top(),
                          buttonWidth - (margins.left() + padding * 3 + iconWidth) - padding, buttonHeight);
    QFontMetrics fm( font() );
    painter.drawText( textRect, Qt::AlignVCenter | Qt::TextWordWrap, text() );
}


void
AnimatedBarWidget::drawHoverBackground(QPainter* painter)
{
    const bool isHovered = isHoverHintEnabled();
    if( isHovered )
    {
        QStyleOptionViewItem option;
        option.initFrom(this);
        option.state = QStyle::State_Enabled | QStyle::State_Selected;
        option.viewItemPosition = QStyleOptionViewItem::OnlyOne;
        style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter, this );
    }
    else
    {
        QStyleOptionViewItem option;
        option.initFrom(this);
        option.state = QStyle::State_Enabled | QStyle::State_MouseOver;
        option.viewItemPosition = QStyleOptionViewItem::OnlyOne;
        style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter, this );
    }
}

QColor
AnimatedBarWidget::foregroundColor() const
{
    const bool isHighlighted = isHoverHintEnabled();

    QColor foregroundColor = palette().color( foregroundRole() );
    if( !isHighlighted )
        foregroundColor.setAlpha( 60 );

    return foregroundColor;
}


QSize
AnimatedBarWidget::sizeHint() const
{
    QSize size = QAbstractButton::sizeHint();
    size.setHeight( iconSize().height() + 8 );
    return size;
}

/**
 * Find the height required to fit the widget with wrapped text
 * and the icon, plus padding
 */
int
AnimatedBarWidget::heightForWidth(int w) const
{
    const int hPadding = 2;
    const int vPadding = 4;
    // Padding to the left and right of both the icon and text
    const QRect bound( 0, 0, (w - hPadding * 4 - iconSize().width()), 0 );
    QFontMetrics fm( font() );
    int fontHeight = fm.boundingRect( bound, Qt::TextWordWrap, text() ).height();
    if( fontHeight < iconSize().height() )
        return iconSize().height() + 2 * vPadding;

    return fontHeight + 2 * vPadding;
}
