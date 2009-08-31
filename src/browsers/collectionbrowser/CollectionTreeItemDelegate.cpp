/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "CollectionTreeItemDelegate.h"
#include "CollectionTreeItem.h"

#include "App.h"
#include "Debug.h"

#include <QAction>
#include <QApplication>
#include <QFontMetrics>
#include <QIcon>
#include <QPainter>
#include <QMenu>
#include <QToolButton>

#include <kcapacitybar.h>

Q_DECLARE_METATYPE( QList<QAction*> )

#define CAPACITYRECT_HEIGHT 6

CollectionTreeItemDelegate::CollectionTreeItemDelegate( QTreeView *view )
    : KWidgetItemDelegate( view )
    , m_view( view )
{
    DEBUG_BLOCK

    m_bigFont.setBold( true );
    m_smallFont.setPointSize( m_smallFont.pointSize() - 1 );
}

CollectionTreeItemDelegate::~CollectionTreeItemDelegate()
{}

void
CollectionTreeItemDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    if( index.parent().isValid() ) // not a root item
    {
        m_styledDelegate.paint( painter, option, index );
        return;
    }

    const bool isRTL = QApplication::isRightToLeft();
    const QPoint topLeft = option.rect.topLeft();
    const int width = m_view->viewport()->size().width() - 4;
    const int height = sizeHint( option, index ).height();
    const int iconWidth = 32;
    const int iconHeight = 32;
    const int iconPadX = 4;
    const bool hasCapacity = index.data( CustomRoles::HasCapacityRole ).toBool();
    const bool hasActions = index.data( CustomRoles::HasDecoratorsRole ).toBool();

    painter->save();

    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );

    if ( option.state & QStyle::State_Selected )
        painter->setPen( App::instance()->palette().highlightedText().color() );
    else
        painter->setPen( App::instance()->palette().text().color() );

    painter->setRenderHint( QPainter::Antialiasing );

    const int iconYPadding = ( height - iconHeight ) / 2;
    QPoint iconPos( topLeft + QPoint( iconPadX, iconYPadding ) );
    if( isRTL )
        iconPos.setX( width - iconWidth - iconPadX );

    painter->drawPixmap( iconPos,
                         index.data( Qt::DecorationRole ).value<QIcon>().pixmap( iconWidth, iconHeight ) );

    const QString collectionName = index.data( Qt::DisplayRole ).toString();
    const QString bylineText = index.data( CustomRoles::ByLineRole ).toString();
    QFontMetrics bigFm( m_bigFont );
    QFontMetrics smallFm( m_smallFont );

    const int actionsRectWidth = hasActions ? 30 : 0;

    const int iconRight = topLeft.x() + iconWidth + iconPadX * 2;
    const int infoRectLeft = isRTL ? actionsRectWidth : iconRight;
    const int infoRectWidth = width - iconRight;

    const int titleRectWidth = infoRectWidth - actionsRectWidth;

    QRectF titleRect;
    titleRect.setLeft( infoRectLeft );
    titleRect.setTop( option.rect.top() + iconYPadding );
    titleRect.setWidth( titleRectWidth );
    titleRect.setHeight( bigFm.boundingRect( collectionName ).height() );

    painter->setFont( m_bigFont );
    painter->drawText( titleRect, Qt::AlignLeft, collectionName );

    QRectF textRect;
    textRect.setLeft( infoRectLeft );
    textRect.setTop( titleRect.bottom() );
    textRect.setWidth( titleRectWidth );
    textRect.setHeight( smallFm.boundingRect( bylineText ).height() );

    painter->setFont( m_smallFont );
    painter->drawText( textRect, Qt::TextWordWrap, bylineText );

    const bool isHover = option.state & QStyle::State_MouseOver;
    QPoint cursorPos = m_view->mapFromGlobal( QCursor::pos() );
    cursorPos.ry() -= 20; // Where the fuck does this offset come from. I have _ZERO_ idea.

    if( hasCapacity )
    {
        QRect capacityRect;
        capacityRect.setLeft( isRTL ? 0 : infoRectLeft );
        capacityRect.setTop( textRect.bottom() );
        capacityRect.setWidth( infoRectWidth );
        capacityRect.setHeight( CAPACITYRECT_HEIGHT );

        const int used = index.data( CustomRoles::UsedCapacityRole ).toInt();

        KCapacityBar capacityBar( KCapacityBar::DrawTextInline );
        capacityBar.setValue( used );

        // TODO: set text in a tooltip where we can show extra info (eg bytes available, not just percentage)
        if( isHover && capacityRect.contains( cursorPos ) )
            capacityBar.setText( i18n("%1% used").arg( QString::number(used) ) );
        capacityBar.drawCapacityBar( painter, capacityRect );
    }

    painter->restore();
}

QList<QWidget*>
CollectionTreeItemDelegate::createItemWidgets() const
{
    QList<QEvent::Type> blockedEvents;
    blockedEvents << QEvent::MouseButtonPress
                  << QEvent::MouseButtonRelease
                  << QEvent::MouseButtonDblClick;

    QToolButton *button = new QToolButton();
    setBlockedEventTypes( button, blockedEvents );

    return QList<QWidget*>() << button;
}

void
CollectionTreeItemDelegate::updateItemWidgets( const QList<QWidget*> widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index ) const
{
    const QList<QAction*> actions = index.data( CustomRoles::DecoratorsRole ).value< QList<QAction*> >();

    if( index.parent().isValid() || actions.isEmpty() )
    {
        foreach( QWidget *w, widgets )
        {
            if( w )
                w->hide();
        }
        return;
    }

    QToolButton *toolButton = static_cast<QToolButton*>( widgets[0] );
    toolButton->setDefaultAction( actions.first() );

    if( !toolButton->menu() && actions.size() > 1 )
    {
        QMenu *menu = new QMenu( toolButton );
        foreach( QAction *action, actions )
            menu->addAction( action );

        toolButton->setMenu( menu );
        toolButton->setPopupMode( QToolButton::MenuButtonPopup );
    }

    toolButton->setAutoFillBackground( false );
    toolButton->setAutoRaise( true );

    debug() << "Moving toolbutton for" << index.data( Qt::DisplayRole ).toString() << "to:"
            << (option.rect.width() - toolButton->size().width() - 10) << ","
            << (sizeHint( option, index ).height() / 2 - toolButton->size().height() / 2);

    toolButton->move( option.rect.width() - toolButton->size().width() - 10,
                      sizeHint( option, index ).height() / 2 - toolButton->size().height() / 2 );
}

QSize
CollectionTreeItemDelegate::sizeHint( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    if( index.parent().isValid() )
        return m_styledDelegate.sizeHint( option, index );

    int width = m_view->viewport()->size().width() - 4;
    int height;
    const bool hasCapacity = index.data( CustomRoles::HasCapacityRole ).toBool();

    QFontMetrics bigFm( m_bigFont );
    QFontMetrics smallFm( m_smallFont );

    height = bigFm.boundingRect( 0, 0, width, 50, Qt::AlignLeft, index.data( Qt::DisplayRole ).toString() ).height()
           + smallFm.boundingRect( 0, 0, width, 50, Qt::AlignLeft, index.data( CustomRoles::ByLineRole ).toString() ).height()
           + (hasCapacity ? CAPACITYRECT_HEIGHT : 0)
           + 20;

    return QSize( width, height );
}

