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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "CollectionTreeItemDelegate.h"
#include "CollectionTreeItem.h"

#include "App.h"
#include "Debug.h"

#include "context/popupdropper/libpud/PopupDropperAction.h"

#include <QAction>
#include <QApplication>
#include <QFontMetrics>
#include <QIcon>
#include <QPainter>

#include <kcapacitybar.h>

Q_DECLARE_METATYPE( QList<PopupDropperAction*> )

#define CAPACITYRECT_HEIGHT 6
#define ACTIONICON_SIZE 24

CollectionTreeItemDelegate::CollectionTreeItemDelegate( QTreeView *view )
    : QStyledItemDelegate( view )
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
        QStyledItemDelegate::paint( painter, option, index );
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
    const QList<PopupDropperAction*> actions = index.data( CustomRoles::DecoratorsRole ).value< QList<PopupDropperAction*> >();

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
        iconPos = QPoint( width - iconWidth - iconPadX, iconYPadding );

    painter->drawPixmap( iconPos,
                         index.data( Qt::DecorationRole ).value<QIcon>().pixmap( iconWidth, iconHeight ) );

    const QString collectionName = index.data( Qt::DisplayRole ).toString();
    const QString bylineText = index.data( CustomRoles::ByLineRole ).toString();
    QFontMetrics bigFm( m_bigFont );
    QFontMetrics smallFm( m_smallFont );

    const int iconRight = topLeft.x() + iconWidth + iconPadX * 2;
    const int infoRectLeft = isRTL ? 0 : iconRight;
    const int infoRectWidth = width - iconRight;

    const int actionsRectWidth = hasActions ? actions.size() * ACTIONICON_SIZE : 0;

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

    if( hasCapacity )
    {
        QRect capacityRect;
        capacityRect.setLeft( infoRectLeft );
        capacityRect.setTop( textRect.bottom() );
        capacityRect.setWidth( infoRectWidth );
        capacityRect.setHeight( CAPACITYRECT_HEIGHT );

        const int used = index.data( CustomRoles::UsedCapacityRole ).toInt();

        KCapacityBar capacityBar( KCapacityBar::DrawTextInline );
        capacityBar.setValue( used );
        // TODO: set text in a tooltip where we can show extra info (eg bytes available, not just percentage)
        //capacityBar.setText( i18n("%1% used").arg( QString::number(used) ) );
        capacityBar.drawCapacityBar( painter, capacityRect );
    }

    if( hasActions )
    {
        QRect actionsRect;
        actionsRect.setLeft( titleRect.right() );
        actionsRect.setTop( titleRect.top() );
        actionsRect.setWidth( actionsRectWidth );
        actionsRect.setHeight( ACTIONICON_SIZE );

        QPoint actionTopLeft = actionsRect.topLeft();
        const QSize iconSize = QSize( ACTIONICON_SIZE, ACTIONICON_SIZE );

        const bool isHover = option.state & QStyle::State_MouseOver;

        const QPoint cursorPos = m_view->mapFromGlobal( QCursor::pos() );

        foreach( PopupDropperAction *action, actions )
        {
            QIcon icon = action->icon();
            QRect iconRect( actionTopLeft, iconSize );
            debug() << "iconRect:" << iconRect << "cursor:" << cursorPos << "in?" << iconRect.contains( cursorPos );

            const bool isOver = isHover && iconRect.contains( cursorPos );

            icon.paint( painter, iconRect, Qt::AlignCenter, isOver ? QIcon::Active : QIcon::Normal, isOver ? QIcon::On : QIcon::Off );
            actionTopLeft.rx() += ACTIONICON_SIZE;
        }
    }

    painter->restore();
}

QSize
CollectionTreeItemDelegate::sizeHint( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    if( index.parent().isValid() )
        return QStyledItemDelegate::sizeHint( option, index );

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


