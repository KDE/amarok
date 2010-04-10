/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "PlaylistTreeItemDelegate.h"

#include "MetaPlaylistModel.h"

#include "App.h"
#include "core/support/Debug.h"

#include <QAction>
#include <QApplication>
#include <QFontMetrics>
#include <QIcon>
#include <QPainter>
#include <QMenu>
#include <QToolButton>

#define ACTIONICON_SIZE 16

QHash<QPersistentModelIndex, QRect> PlaylistTreeItemDelegate::s_indexActionsRects;

PlaylistTreeItemDelegate::PlaylistTreeItemDelegate( QTreeView *view )
    : QStyledItemDelegate( view )
    , m_view( view )
{
    DEBUG_BLOCK

    m_bigFont.setBold( true );
    m_smallFont.setPointSize( m_smallFont.pointSize() - 1 );
}

PlaylistTreeItemDelegate::~PlaylistTreeItemDelegate()
{}

void
PlaylistTreeItemDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index ) const
{
    if( index.parent().isValid() ) // not a root item
    {
        QStyledItemDelegate::paint( painter, option, index );
        return;
    }

    const bool isRTL = QApplication::isRightToLeft();
    const QPoint topLeft = option.rect.topLeft();
    const QPoint bottomRight = option.rect.bottomRight();
    const int width = m_view->viewport()->size().width() - 4;
    const int height = sizeHint( option, index ).height();
    const int iconWidth = 32;
    const int iconHeight = 32;
    const int iconPadX = 4;
    const int actionCount
            = index.data( PlaylistBrowserNS::MetaPlaylistModel::ActionCountRole ).toInt();

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
                         index.data( Qt::DecorationRole )
                         .value<QIcon>().pixmap( iconWidth, iconHeight ) );

    QStyleOption expanderOption( option );
    if( isRTL )
        expanderOption.rect.setLeft( iconPadX );
    else
        expanderOption.rect.setLeft( option.rect.right() - iconPadX - iconWidth );

    expanderOption.rect.setWidth( iconWidth );
    if( m_view->model()->hasChildren( index ) )
        expanderOption.state |= QStyle::State_Children;
    if( m_view->isExpanded( index ) )
    {
        expanderOption.state |= QStyle::State_Open;
        //when expanded the sibling indicator (a vertical line down) goes nowhere
        expanderOption.state &= ~QStyle::State_Sibling;
    }

    QApplication::style()->drawPrimitive( QStyle::PE_IndicatorBranch, &expanderOption, painter );

    const QString collectionName = index.data( Qt::DisplayRole ).toString();
    const QString bylineText = index.data(
            PlaylistBrowserNS::MetaPlaylistModel::ByLineRole ).toString();
    QFontMetrics bigFm( m_bigFont );
    QFontMetrics smallFm( m_smallFont );

     const int actionsRectWidth = actionCount > 0 ?
                                  (ACTIONICON_SIZE * actionCount + 2*2/*margin*/):
                                  0;

    const int iconRight = topLeft.x() + iconWidth + iconPadX * 2;
    const int infoRectLeft = isRTL ? actionsRectWidth : iconRight;
    const int infoRectWidth = width - iconRight;
    const int titleRectWidth = infoRectWidth;

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
    cursorPos.ry() -= 20;
    if( actionCount > 0 && isHover )
    {
        //HACK: there is an issue with QtGroupingProxy: a UserValue is returned as multiple copies in a QVariantList. So only take the first.

        QList<QAction*> actions;
        QVariantList actionsVariants =
                index.data( PlaylistBrowserNS::MetaPlaylistModel::ActionRole ).toList();
        if( !actionsVariants.isEmpty() )
            actions = actionsVariants.first().value<QList<QAction*> >();

        QRect actionsRect;
        actionsRect.setLeft( ( width - actionCount * ( ACTIONICON_SIZE + iconPadX ) ) - 2 );
        actionsRect.setTop( option.rect.top() + iconYPadding );
        actionsRect.setWidth( actionsRectWidth );
        actionsRect.setHeight( ACTIONICON_SIZE );

        QPoint actionTopLeftBase = actionsRect.topLeft();
        const QSize iconSize = QSize( ACTIONICON_SIZE, ACTIONICON_SIZE );

        int i = 0;
        foreach( QAction *action, actions )
        {
            QIcon icon = action->icon();
            int x = actionTopLeftBase.x() + i * ( ACTIONICON_SIZE + iconPadX );
            QPoint actionTopLeft = QPoint( x, actionTopLeftBase.y() );
            //TODO: make this work with multiple actions
            QRect iconRect( actionTopLeft, iconSize );

            const bool isOver = isHover && iconRect.contains( cursorPos );

            icon.paint( painter, iconRect, Qt::AlignCenter, isOver ? QIcon::Active : QIcon::Normal,
                        isOver ? QIcon::On : QIcon::Off );
            i++;
        }

        // Store the Model index for lookups for clicks. FAIL.
        QPersistentModelIndex persistentIndex( index );
        s_indexActionsRects.insert( persistentIndex, actionsRect );
    }

    painter->restore();
}

QSize
PlaylistTreeItemDelegate::sizeHint( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    if( index.parent().isValid() )
        return QStyledItemDelegate::sizeHint( option, index );

    int width = m_view->viewport()->size().width() - 4;
    int height;

    QFontMetrics bigFm( m_bigFont );
    QFontMetrics smallFm( m_smallFont );

    height = bigFm.boundingRect( 0, 0, width, 50, Qt::AlignLeft,
                                 index.data( Qt::DisplayRole ).toString() ).height()
           + smallFm.height()
           + 20;

    return QSize( width, height );
}

QRect
PlaylistTreeItemDelegate::actionsRect( const QModelIndex &index )
{
    QPersistentModelIndex persistentIndex( index );
    return s_indexActionsRects.value( persistentIndex );
}
