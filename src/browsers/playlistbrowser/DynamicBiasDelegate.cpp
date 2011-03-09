/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DynamicBiasDelegate.h"
#include "dynamic/DynamicModel.h"

#include "App.h"
// #include "Bias.h"
// #include "core/support/Debug.h"

#include <QApplication>
#include <QPainter>

PlaylistBrowserNS::DynamicBiasDelegate::DynamicBiasDelegate( QWidget* parent )
    : QStyledItemDelegate( parent )
{
    m_bigFont.setBold( true );
    m_smallFont.setPointSize( m_smallFont.pointSize() - 1 );

    m_bigFm = new QFontMetrics( m_bigFont );
    m_normalFm = new QFontMetrics( m_normalFont );
    m_smallFm = new QFontMetrics( m_smallFont );
}

PlaylistBrowserNS::DynamicBiasDelegate::~DynamicBiasDelegate()
{
    delete m_bigFm;
    delete m_normalFm;
    delete m_smallFm;
}

void
PlaylistBrowserNS::DynamicBiasDelegate::paint( QPainter* painter,
                                               const QStyleOptionViewItem& option,
                                               const QModelIndex& index ) const
{
    Dynamic::DynamicPlaylist* playlist = 0;
    Dynamic::AbstractBias* bias = 0;

    QVariant v = index.model()->data( index, Dynamic::DynamicModel::PlaylistRole );
    if( v.isValid() )
        playlist = qobject_cast<Dynamic::DynamicPlaylist*>(v.value<QObject*>() );

    v = index.model()->data( index, Dynamic::DynamicModel::BiasRole );
    if( v.isValid() )
        bias = qobject_cast<Dynamic::AbstractBias*>(v.value<QObject*>() );

    const bool isRTL = QApplication::isRightToLeft();
    // QSize size = sizeHint( option, index );
    const int width = option.rect.size().width();
    const int height = option.rect.size().height();

    if( playlist )
    {
        const QPoint topLeft = option.rect.topLeft();
        const QPoint bottomRight = option.rect.bottomRight();
        // const int width = m_view->viewport()->size().width() - 4;

        const int iconWidth = 32;
        const int iconHeight = 32;
        const int iconPadX = 4;
        // const int actionCount = index.data( CustomRoles::DecoratorRoleCount ).toInt();

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
        /*
           if( m_view->model()->hasChildren( index ) )
           {
           if( m_view->isExpanded( index ) )
           {
           QApplication::style()->drawPrimitive( QStyle::PE_IndicatorArrowDown, &expanderOption,
           painter );
           }
           else
           {
           */
        QApplication::style()->drawPrimitive( expandedPrimitive, &expanderOption,
                                              painter );
        /*
           }
           }
           */

        const QString text = index.data( Qt::DisplayRole ).toString();
        // const QString bylineText = index.data( CustomRoles::ByLineRole ).toString();

        const int actionsRectWidth = 0;
        //  actionCount > 0 ?
        //                             (ACTIONICON_SIZE * actionCount + 2*2/*margin*/) : 0;

        const int iconRight = topLeft.x() + iconWidth + iconPadX * 2;
        const int infoRectLeft = isRTL ? actionsRectWidth : iconRight;
        const int infoRectWidth = width - iconRight;

        const int titleRectWidth = infoRectWidth - actionsRectWidth;

        QRectF titleRect;
        titleRect.setLeft( infoRectLeft );
        titleRect.setTop( option.rect.top() + iconYPadding );
        titleRect.setWidth( titleRectWidth );
        // titleRect.setHeight( m_bigFm->boundingRect( text ).height() );
        titleRect.setHeight( height );

        painter->setFont( m_bigFont );
        painter->drawText( titleRect, Qt::AlignLeft | Qt::AlignVCenter, text );

        QRectF textRect;
        textRect.setLeft( infoRectLeft );
        textRect.setTop( titleRect.bottom() );
        textRect.setWidth( titleRectWidth );
        // textRect.setHeight( m_smallFm->boundingRect( bylineText ).height() );

        painter->setFont( m_smallFont );
        // painter->drawText( textRect, Qt::TextWordWrap, bylineText );

        // const bool isHover = option.state & QStyle::State_MouseOver;
        // QPoint cursorPos = m_view->mapFromGlobal( QCursor::pos() );
        // cursorPos.ry() -= 20; // Where the fuck does this offset come from. I have _ZERO_ idea.

        //show actions when there are any and mouse is hovering over item
        /*
           if( actionCount > 0 && isHover )
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
        */

        painter->restore();
    }
    else if( bias )
    {
        QModelIndex parentIndex = index.parent();
        Dynamic::AbstractBias* parentBias = 0;

        v = parentIndex.model()->data( parentIndex, Dynamic::DynamicModel::BiasRole );
        if( v.isValid() )
            parentBias = qobject_cast<Dynamic::AbstractBias*>(v.value<QObject*>() );

        if( parentBias )
        {
            // sub-biases have a operator drawn in front of them.
            const int operatorWidth = m_smallFm->boundingRect( "mmmm" ).width();

            // draw the selection
            QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );

            // TODO: isRTL

            // -- paint the operator
            QRect operatorRect( option.rect.x(), option.rect.y(),
                                operatorWidth, option.rect.height() );
            painter->setFont( m_smallFont );
            parentBias->paintOperator( painter, operatorRect, bias );

            // -- paint the normal text
            QRect textRect( option.rect.x() + operatorWidth, option.rect.y(),
                            option.rect.width() - operatorWidth, option.rect.height() );
            painter->setFont( m_normalFont );
            const QString text = index.data( Qt::DisplayRole ).toString();
            painter->drawText( textRect, Qt::TextWordWrap, text );
        }
        else
        {
            QStyledItemDelegate::paint( painter, option, index );
        }

    }
    else
    {
        QStyledItemDelegate::paint( painter, option, index );
    }

}



QSize
PlaylistBrowserNS::DynamicBiasDelegate::sizeHint( const QStyleOptionViewItem& option,
                                                  const QModelIndex& index ) const
{
    Dynamic::DynamicPlaylist* playlist = 0;
    QVariant v = index.model()->data( index, Dynamic::DynamicModel::PlaylistRole );
    if( v.isValid() )
        playlist = qobject_cast<Dynamic::DynamicPlaylist*>(v.value<QObject*>() );

    QSize size = QStyledItemDelegate::sizeHint( option, index );

    if( !playlist )
        return size;

    const int iconWidth = 32;
    const int iconHeight = 32;
    const int iconPadX = 4;

    QSize iconSize = QSize( iconWidth + iconPadX, iconHeight + 4 );
    QSize textSize = m_bigFm->boundingRect( 0, 0,
                                            size.width() - iconSize.width(), 50,
                                            Qt::AlignLeft,
                                            index.data( Qt::DisplayRole ).toString() ).size();

    return size.expandedTo( textSize ).expandedTo( iconSize );

        /*
        + m_smallFm->boundingRect( 0, 0, width, 50, Qt::AlignLeft,
                                   index.data( CustomRoles::ByLineRole ).toString() ).height()
        + 20;

    return QSize( width, height );
                                   */
}

