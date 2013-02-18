/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include <kcapacitybar.h>

#include <QAction>
#include <QApplication>
#include <QFontMetrics>
#include <QIcon>
#include <QPainter>

Q_DECLARE_METATYPE( QAction* )
Q_DECLARE_METATYPE( QList<QAction*> )

#define CAPACITYRECT_MIN_HEIGHT 12
#define CAPACITYRECT_MAX_HEIGHT 18

QHash<QPersistentModelIndex, QRect> CollectionTreeItemDelegate::s_indexDecoratorRects;

CollectionTreeItemDelegate::CollectionTreeItemDelegate( QTreeView *view )
    : QStyledItemDelegate( view )
    , m_view( view )
    , m_normalFm( 0 )
    , m_bigFm( 0 )
    , m_smallFm( 0 )
{
    Q_ASSERT( m_view );
}

CollectionTreeItemDelegate::~CollectionTreeItemDelegate()
{
    delete m_normalFm;
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

    updateFonts( option );

    QStyle *style;
    if( QWidget *w = qobject_cast<QWidget*>(parent()) )
        style = w->style();
    else
        style = QApplication::style();

    const int verticalSpace = qMax( style->pixelMetric( QStyle::PM_LayoutVerticalSpacing ), 1 );
    const int largeIconSize = style->pixelMetric( QStyle::PM_LargeIconSize );
    const int expanderIconSize = style->pixelMetric( QStyle::PM_MenuButtonIndicator );
    const int tinyIconSize = qMax( style->pixelMetric( QStyle::PM_ListViewIconSize ), largeIconSize / 2 );
    const int frameHMargin = style->pixelMetric( QStyle::PM_FocusFrameHMargin );
    const int frameVMargin = style->pixelMetric( QStyle::PM_FocusFrameVMargin );
    const int iconSpacing = style->pixelMetric( QStyle::PM_FocusFrameHMargin );

    const bool isRTL = QApplication::isRightToLeft();
    const bool hasCapacity = index.data( CustomRoles::HasCapacityRole ).toBool();
    const int actionCount = index.data( CustomRoles::DecoratorRoleCount ).toInt();

    QRect remainingRect( option.rect );
    remainingRect.adjust( frameHMargin, frameVMargin, -frameHMargin, -frameVMargin );

    painter->save();

    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );

    if ( option.state & QStyle::State_Selected )
        painter->setPen( App::instance()->palette().highlightedText().color() );
    else
        painter->setPen( App::instance()->palette().text().color() );

    painter->setRenderHint( QPainter::Antialiasing );

    // -- the icon
    // we need an additional 2 px margin or else the icons will look oversized as compared
    // to the album covers (which get a 2 px border)
    const int iconYPadding = ( remainingRect.height() - largeIconSize ) / 2;
    QPoint iconPos( remainingRect.topLeft() + QPoint( 2, iconYPadding + 2 ) );
    if( isRTL )
        iconPos.setX( remainingRect.right() - largeIconSize - 2 );

    painter->drawPixmap( iconPos,
                         index.data( Qt::DecorationRole ).value<QIcon>().pixmap( largeIconSize - 4, largeIconSize - 4 ) );

    if( isRTL )
        remainingRect.adjust( 0, 0, - largeIconSize - iconSpacing, 0 );
    else
        remainingRect.adjust( largeIconSize + iconSpacing, 0, 0, 0 );

    // -- expander option (the small arrow)
    // const int expanderYPadding = ( remainingRect.height() - expanderIconSize ) / 2;
    QStyleOption expanderOption( option );
    expanderOption.rect = remainingRect;
    QStyle::PrimitiveElement expandedPrimitive;
    if( isRTL )
    {
        expandedPrimitive = QStyle::PE_IndicatorArrowRight;
    }
    else
    {
        expandedPrimitive = QStyle::PE_IndicatorArrowLeft;
        expanderOption.rect.setLeft( expanderOption.rect.right() - expanderIconSize );
        expanderOption.rect.setWidth( expanderIconSize );
    }

    expanderOption.rect.setWidth( expanderIconSize );
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

    // always substract the expander size in order to align all the rest
    if( isRTL )
        remainingRect.adjust( expanderIconSize + iconSpacing, 0, 0, 0 );
    else
        remainingRect.adjust( 0, 0, - expanderIconSize - iconSpacing, 0 );

    // -- title
    const QString collectionName = index.data( Qt::DisplayRole ).toString();

    QRect titleRect( remainingRect );
    titleRect.setHeight( qMax( tinyIconSize, m_bigFm->height() ) + verticalSpace );

    painter->setFont( m_bigFont );
    painter->drawText( titleRect, Qt::AlignLeft, collectionName );

    const bool isHover = option.state & QStyle::State_MouseOver;
    // The collectionBrowserTreeView triggers repaints if the mouse moves so
    // that we always get updated mouse positions
    QPoint cursorPos = m_view->mapFromGlobal( QCursor::pos() );
    cursorPos.ry() -= 20; // Where the fuck does this offset come from. I have _ZERO_ idea.

    // -- actions icons
    if( isHover && ( actionCount > 0 ) )
    {
        const QList<QAction*> actions =
                index.data( CustomRoles::DecoratorRole ).value<QList<QAction*> >();

        const int actionsRectWidth = actionCount > 0 ?
            ( actionCount * ( tinyIconSize + iconSpacing ) - iconSpacing ) : 0;

        QRect decoratorRect( titleRect );
        if( ! isRTL )
            decoratorRect.setLeft( titleRect.right() - actionsRectWidth );
        decoratorRect.setHeight( tinyIconSize );
        decoratorRect.setWidth( actionsRectWidth );

        QRect iconRect( decoratorRect.topLeft(), QSize( tinyIconSize, tinyIconSize ) );
        foreach( QAction* action, actions )
        {
            const bool isOver = isHover && iconRect.contains( cursorPos );

            action->icon().paint( painter, iconRect, Qt::AlignCenter,
                                  isOver ? QIcon::Active : QIcon::Normal,
                                  isOver ? QIcon::On : QIcon::Off );

            iconRect.adjust( tinyIconSize + iconSpacing, 0, tinyIconSize + iconSpacing, 0 );
        }

        // Store the Model index for lookups for clicks. FAIL.
        QPersistentModelIndex persistentIndex( index );
        s_indexDecoratorRects.insert( persistentIndex, decoratorRect );
    }


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

        QRect capacityRect( remainingRect );
        capacityRect.setTop( titleRect.bottom() );
        capacityRect.setHeight( qBound( CAPACITYRECT_MIN_HEIGHT,
                                        capacityBar.minimumSizeHint().height(),
                                        qMin( CAPACITYRECT_MAX_HEIGHT, capacityRect.height() ) ) );

        capacityBar.drawCapacityBar( painter, capacityRect );
    }
    else
    {
        const QString bylineText = index.data( CustomRoles::ByLineRole ).toString();

        QRectF textRect( remainingRect );
        textRect.setTop( titleRect.bottom() );
        textRect.setHeight( remainingRect.height() - titleRect.height() );

        painter->drawText( textRect, Qt::TextWordWrap, bylineText );
    }

    painter->restore();
}

QSize
CollectionTreeItemDelegate::sizeHint( const QStyleOptionViewItem & option,
                                      const QModelIndex & index ) const
{
    // note: the QStyledItemDelegage::sizeHint seems to be extremly slow. don't call it

    QStyle *style;
    if( QWidget *w = qobject_cast<QWidget*>(parent()) )
        style = w->style();
    else
        style = QApplication::style();

    updateFonts( option );

    const int verticalSpace = qMax( style->pixelMetric( QStyle::PM_LayoutVerticalSpacing ), 1 );
    const int largeIconSize = style->pixelMetric( QStyle::PM_LargeIconSize );
    const int expanderIconSize = style->pixelMetric( QStyle::PM_MenuButtonIndicator );
    const int tinyIconSize = qMax( style->pixelMetric( QStyle::PM_ListViewIconSize ), largeIconSize / 2 );
    const int frameHMargin = style->pixelMetric( QStyle::PM_FocusFrameHMargin );
    const int frameVMargin = style->pixelMetric( QStyle::PM_FocusFrameVMargin );
    const int iconSpacing = style->pixelMetric( QStyle::PM_FocusFrameHMargin );

    int viewportWidth = m_view->viewport()->size().width();
    int normalHeight = frameVMargin + qMax( tinyIconSize, m_normalFm->height() ) + frameVMargin;


    // -- determine if we have an album
    const bool hasCover = index.data( CustomRoles::HasCoverRole ).toBool();

    if( hasCover )
        return QSize( viewportWidth, qMax( normalHeight, largeIconSize + frameVMargin * 2 ) );


    // -- not top level. this is a normal item
    if( index.parent().isValid() )
        return QSize( viewportWidth, normalHeight );

    // -- ok, we have a top level item
    const bool hasCapacity = index.data( CustomRoles::HasCapacityRole ).toBool();
    const int actionCount = index.data( CustomRoles::DecoratorRoleCount ).toInt();

    QSize iconSize( largeIconSize, largeIconSize );
    QSize expanderSize( expanderIconSize, expanderIconSize );
    QSize titleSize( m_bigFm->boundingRect( index.data( Qt::DisplayRole ).toString() ).size() );
    QSize capacitySize( hasCapacity ? 10 : 0, hasCapacity ? CAPACITYRECT_MIN_HEIGHT : 0 );

    const QList<QAction*> actions =
        index.data( CustomRoles::DecoratorRole ).value<QList<QAction*> >();

    const int actionsRectWidth = actionCount > 0 ?
        ( actionCount * ( tinyIconSize + iconSpacing ) - iconSpacing ) : 0;
    QSize actionIconsSize( actionsRectWidth, actionCount > 0 ? tinyIconSize : 0 );

    int layoutWidth = frameHMargin +
        iconSize.width() + iconSpacing + titleSize.width() +
        actionIconsSize.width() + iconSpacing + expanderSize.width() +
        frameHMargin;

    // if we have more size, use it (in order to get more space for the
    // word wrapping byline
    layoutWidth = qMax( viewportWidth, layoutWidth );

    QSize bylineSize( m_smallFm->boundingRect( 0, 0, layoutWidth, 300, Qt::TextWordWrap,
                                               index.data( CustomRoles::ByLineRole ).toString() ).size() );

    int layoutHeight = frameVMargin +
        qMax( titleSize.height(), actionIconsSize.height() ) + verticalSpace +
        qMax( capacitySize.height(), bylineSize.height() ) +
        frameVMargin;

    return QSize( layoutWidth, layoutHeight );
}

QRect
CollectionTreeItemDelegate::decoratorRect( const QModelIndex &index )
{
    QPersistentModelIndex persistentIndex( index );
    return s_indexDecoratorRects.value( persistentIndex );
}

void
CollectionTreeItemDelegate::updateFonts( const QStyleOptionViewItem &option ) const
{
    if( m_normalFm && m_bigFm && m_smallFm && option.font == m_originalFont )
        return;

    m_originalFont = option.font;

    delete m_normalFm;
    m_normalFm = new QFontMetrics( m_originalFont );

    m_bigFont = m_originalFont;
    m_bigFont.setBold( true );
    delete m_bigFm;
    m_bigFm = new QFontMetrics( m_bigFont );

    m_smallFont = m_originalFont;
    m_smallFont.setPointSize( m_smallFont.pointSize() - 1 );
    delete m_smallFm;
    m_smallFm = new QFontMetrics( m_smallFont );
}
