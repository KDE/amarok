/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "CollectionTreeItemDelegate.h"

#include "App.h"
#include "browsers/CollectionTreeItem.h"
#include "core/support/Debug.h"

#include <kcapacitybar.h>

#include <QAction>
#include <QApplication>
#include <QFontMetrics>
#include <QIcon>
#include <QPainter>
#include <QMenu>
#include <QToolButton>

Q_DECLARE_METATYPE( QAction* )
Q_DECLARE_METATYPE( QList<QAction*> )

#define CAPACITYRECT_MIN_HEIGHT 12
#define CAPACITYRECT_MAX_HEIGHT 18
#define ACTIONICON_SIZE 16

QHash<QPersistentModelIndex, QRect> CollectionTreeItemDelegate::s_indexDecoratorRects;

CollectionTreeItemDelegate::CollectionTreeItemDelegate( QTreeView *view )
    : QStyledItemDelegate( view )
    , m_view( view )
{
    DEBUG_BLOCK

    m_bigFont.setBold( true );
    m_smallFont.setPointSize( m_smallFont.pointSize() - 1 );

    m_bigFm = new QFontMetrics( m_bigFont );
    m_smallFm = new QFontMetrics( m_smallFont );
}

CollectionTreeItemDelegate::~CollectionTreeItemDelegate()
{
    delete m_bigFm;
    delete m_smallFm;
}

void
CollectionTreeItemDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index ) const
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
    const int actionCount = index.data( CustomRoles::DecoratorRoleCount ).toInt();

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

    QStyleOption expanderOption( option );
    QStyle::PrimitiveElement expandedPrimitive;
    if( isRTL )
    {
        expandedPrimitive = QStyle::PE_IndicatorArrowLeft;
        expanderOption.rect.setLeft( iconPadX );
    }
    else
    {
        expandedPrimitive = QStyle::PE_IndicatorArrowRight;
        expanderOption.rect.setLeft( option.rect.right() - iconPadX - iconWidth );
    }

    expanderOption.rect.setWidth( iconWidth );
    //FIXME: CollectionTreeItemModelBase::hasChildren() returns true for root items regardless
    if( m_view->model()->hasChildren( index ) )
    {
        if( m_view->isExpanded( index ) )
        {
            QApplication::style()->drawPrimitive( QStyle::PE_IndicatorArrowDown, &expanderOption,
                                                  painter );
        }
        else
        {
            QApplication::style()->drawPrimitive( expandedPrimitive, &expanderOption,
                                                  painter );
        }
    }

    const QString collectionName = index.data( Qt::DisplayRole ).toString();
    const QString bylineText = index.data( CustomRoles::ByLineRole ).toString();

    const int actionsRectWidth = actionCount > 0 ?
                                 (ACTIONICON_SIZE * actionCount + 2*2/*margin*/) : 0;

    const int iconRight = topLeft.x() + iconWidth + iconPadX * 2;
    const int infoRectLeft = isRTL ? actionsRectWidth : iconRight;
    const int infoRectWidth = width - iconRight;

    const int titleRectWidth = infoRectWidth - actionsRectWidth;

    QRectF titleRect;
    titleRect.setLeft( infoRectLeft );
    titleRect.setTop( option.rect.top() + iconYPadding );
    titleRect.setWidth( titleRectWidth );
    titleRect.setHeight( m_bigFm->boundingRect( collectionName ).height() );

    painter->setFont( m_bigFont );
    painter->drawText( titleRect, Qt::AlignLeft, collectionName );

    const bool isHover = option.state & QStyle::State_MouseOver;
    QPoint cursorPos = m_view->mapFromGlobal( QCursor::pos() );
    cursorPos.ry() -= 20; // Where the fuck does this offset come from. I have _ZERO_ idea.

    painter->setFont( m_smallFont );  // we want smaller font for both subtitle and capacity bar
    //show the bylinetext or the capacity (if available) when hovering
    if( isHover && hasCapacity )
    {
        qreal bytesUsed = index.data( CustomRoles::UsedCapacityRole ).toReal();
        qreal bytesTotal = index.data( CustomRoles::TotalCapacityRole ).toReal();
        const int percentage = (bytesTotal > 0.0) ? qRound( 100.0 * bytesUsed / bytesTotal ) : 100;

        KCapacityBar capacityBar( KCapacityBar::DrawTextInline );
        capacityBar.setValue( percentage );
        capacityBar.setText( i18nc( "Example: 3.5 GB free (unit is part of %1)", "%1 free",
                                    KGlobal::locale()->formatByteSize( bytesTotal - bytesUsed, 1 ) ) );

        QRect capacityRect;
        capacityRect.setLeft( isRTL ? 0 : infoRectLeft );
        capacityRect.setTop( titleRect.bottom() );
        //makeing sure capacity bar does not overlap expander
        capacityRect.setWidth( infoRectWidth - iconWidth );
        capacityRect.setHeight( qBound( CAPACITYRECT_MIN_HEIGHT,
                                        capacityBar.minimumSizeHint().height(),
                                        CAPACITYRECT_MAX_HEIGHT ) );

        capacityBar.drawCapacityBar( painter, capacityRect );
    }
    else
    {
        QRectF textRect;
        textRect.setLeft( infoRectLeft );
        textRect.setTop( titleRect.bottom() );
        textRect.setWidth( titleRectWidth );
        textRect.setHeight( m_smallFm->boundingRect( bylineText ).height() );

        painter->drawText( textRect, Qt::TextWordWrap, bylineText );
    }

    if( isHover && ( actionCount > 0 ) )
    {
        const QList<QAction*> actions =
                index.data( CustomRoles::DecoratorRole ).value<QList<QAction*> >();
        QRect decoratorRect;
        if( isRTL )
            //actions should appear to the right of the expander
            decoratorRect.setLeft( expanderOption.rect.right() + iconPadX );
        else
            //actions should appear left of the expander
            decoratorRect.setLeft( expanderOption.rect.left() -
                                   actionCount * ( ACTIONICON_SIZE + iconPadX ) );
        decoratorRect.setTop( option.rect.top() + iconYPadding );
        decoratorRect.setWidth( actionsRectWidth );
        decoratorRect.setHeight( ACTIONICON_SIZE );

        QPoint actionTopLeftBase = decoratorRect.topLeft();
        const QSize iconSize = QSize( ACTIONICON_SIZE, ACTIONICON_SIZE );

        int i = 0;
        foreach( QAction * action, actions )
        {
            QIcon icon = action->icon();
            int x = actionTopLeftBase.x() + i * ( ACTIONICON_SIZE + iconPadX );
            QPoint actionTopLeft = QPoint( x, actionTopLeftBase.y() );
            QRect iconRect( actionTopLeft, iconSize );

            const bool isOver = isHover && iconRect.contains( cursorPos );

            icon.paint( painter, iconRect, Qt::AlignCenter, isOver ?
                        QIcon::Active : QIcon::Normal, isOver ? QIcon::On : QIcon::Off );
            i++;
        }

        // Store the Model index for lookups for clicks. FAIL.
        QPersistentModelIndex persistentIndex( index );
        s_indexDecoratorRects.insert( persistentIndex, decoratorRect );
    }

    painter->restore();
}

QSize
CollectionTreeItemDelegate::sizeHint( const QStyleOptionViewItem & option,
                                      const QModelIndex & index ) const
{
    if( index.parent().isValid() )
        return QStyledItemDelegate::sizeHint( option, index );

    int width = m_view->viewport()->size().width() - 4;
    int height = m_bigFm->boundingRect( 0, 0, width, 50, Qt::AlignLeft,
                        index.data( Qt::DisplayRole ).toString() ).height()
                 + m_smallFm->boundingRect( 0, 0, width, 50, Qt::AlignLeft,
                        index.data( CustomRoles::ByLineRole ).toString() ).height()
                 + 20;

    return QSize( width, height );
}

QRect
CollectionTreeItemDelegate::decoratorRect( const QModelIndex &index )
{
    QPersistentModelIndex persistentIndex( index );
    return s_indexDecoratorRects.value( persistentIndex );
}
