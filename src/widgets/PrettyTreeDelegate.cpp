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

#include "PrettyTreeDelegate.h"

#include "App.h"
#include "widgets/PrettyTreeRoles.h"
#include "widgets/PrettyTreeView.h"

#include <KCapacityBar>
#include <KFormat>
#include <KLocalizedString>

#include <QAction>
#include <QApplication>
#include <QFontMetrics>
#include <QIcon>
#include <QPainter>
#include <QTreeView>
#include <QHeaderView>

Q_DECLARE_METATYPE( QAction* )
Q_DECLARE_METATYPE( QList<QAction*> )

#define CAPACITYRECT_MIN_HEIGHT 12
#define CAPACITYRECT_MAX_HEIGHT 18

using namespace Amarok;

/**
 * A structure to hold some pixel metrics from style, to allow code sharing
 */
struct PixelSizes
{
    explicit PixelSizes( const QStyle *style )
        : verticalSpace( qMax( style->pixelMetric( QStyle::PM_LayoutVerticalSpacing ), 1 ) )
        , largeIconSize( style->pixelMetric( QStyle::PM_LargeIconSize ) )
        , expanderIconSize( style->pixelMetric( QStyle::PM_MenuButtonIndicator ) )
        , smallIconSize( style->pixelMetric( QStyle::PM_ListViewIconSize ) )
        , frameHMargin( style->pixelMetric( QStyle::PM_FocusFrameHMargin ) )
        , frameVMargin( style->pixelMetric( QStyle::PM_FocusFrameVMargin ) )
        , frameExtraMargin( largeIconSize / 4 ) // to give top items a little more space
        , iconSpacing( style->pixelMetric( QStyle::PM_FocusFrameHMargin ) )
    {}

    const int verticalSpace;
    const int largeIconSize;
    const int expanderIconSize;
    const int smallIconSize;
    const int frameHMargin;
    const int frameVMargin;
    const int frameExtraMargin;
    const int iconSpacing;
};

PrettyTreeDelegate::PrettyTreeDelegate( PrettyTreeView *view )
    : QStyledItemDelegate( view )
    , m_view( view )
    , m_normalFm( nullptr )
    , m_bigFm( nullptr )
    , m_smallFm( nullptr )
{
    Q_ASSERT( m_view );
}

PrettyTreeDelegate::~PrettyTreeDelegate()
{
    delete m_normalFm;
    delete m_bigFm;
    delete m_smallFm;
}

void
PrettyTreeDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index ) const
{
    const bool hasCover = index.data( PrettyTreeRoles::HasCoverRole ).toBool();

    if( hasCover ||
        index.parent().isValid() ) // not a root item
    {
        QStyledItemDelegate::paint( painter, option, index );
        return;
    }

    updateFonts( option );
    QStyle *style = m_view->style();

    PixelSizes s( style );
    const bool isRTL = QApplication::isRightToLeft();
    const bool hasCapacity = index.data( PrettyTreeRoles::HasCapacityRole ).toBool();
    const int actionCount = index.data( PrettyTreeRoles::DecoratorRoleCount ).toInt();

    QRect remainingRect( option.rect );
    remainingRect.adjust( s.frameHMargin,   s.frameVMargin + s.frameExtraMargin,
                          -s.frameHMargin, -s.frameVMargin - s.frameExtraMargin );

    painter->save();
    style->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );

    if ( option.state & QStyle::State_Selected )
        painter->setPen( pApp->palette().highlightedText().color() );
    else
        painter->setPen( pApp->palette().text().color() );

    painter->setRenderHint( QPainter::Antialiasing );

    // -- the icon
    const int iconYPadding = ( remainingRect.height() - s.largeIconSize ) / 2;
    QPoint iconPos( remainingRect.topLeft() + QPoint( 0, iconYPadding ) );
    if( isRTL )
        iconPos.setX( remainingRect.right() - s.largeIconSize );

    painter->drawPixmap( iconPos,
                         index.data( Qt::DecorationRole ).value<QIcon>().pixmap( s.largeIconSize, s.largeIconSize ) );

    if( isRTL )
        remainingRect.adjust( 0, 0, - s.largeIconSize - s.iconSpacing, 0 );
    else
        remainingRect.adjust( s.largeIconSize + s.iconSpacing, 0, 0, 0 );

    // -- expander option (the small arrow)
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
        expanderOption.rect.setLeft( expanderOption.rect.right() - s.expanderIconSize );
    }

    expanderOption.rect.setWidth( s.expanderIconSize );
    if( m_view->model()->hasChildren( index ) )
    {
        if( m_view->isExpanded( index ) )
            style->drawPrimitive( QStyle::PE_IndicatorArrowDown, &expanderOption, painter );
        else
            style->drawPrimitive( expandedPrimitive, &expanderOption, painter );
    }

    // always subtract the expander size in order to align all the rest
    if( isRTL )
        remainingRect.adjust( s.expanderIconSize + s.iconSpacing, 0, 0, 0 );
    else
        remainingRect.adjust( 0, 0, - s.expanderIconSize - s.iconSpacing, 0 );

    // -- title
    QRect titleRect( remainingRect );
    titleRect.setHeight( qMax( m_bigFm->height(), s.smallIconSize ) + s.verticalSpace );

    QString titleText = index.data( Qt::DisplayRole ).toString();
    titleText = m_bigFm->elidedText( titleText, Qt::ElideRight, titleRect.width() );

    painter->setFont( m_bigFont );
    painter->drawText( titleRect, titleText );

    const bool isHover = option.state & QStyle::State_MouseOver;

    // -- actions icons
    if( isHover && ( actionCount > 0 ) )
    {
        const QList<QAction*> actions =
                index.data( PrettyTreeRoles::DecoratorRole ).value<QList<QAction*> >();

        /* The views that have models with action icons set mouse tracking to true, their
         * mouseMoveEvent calls update() which triggers repaints if the mouse moves so
         * that we always get updated mouse positions */
        QPoint cursorPos = m_view->viewport()->mapFromGlobal( QCursor::pos() );
        QAction *pressedDecoratorAction = m_view->pressedDecoratorAction();

        for( int i = 0; i < actions.count(); i++ )
        {
            QRect iconRect( decoratorRect( option.rect, i ) );
            QIcon::State state = ( actions.at( i ) == pressedDecoratorAction ) ? QIcon::On : QIcon::Off;
            QIcon::Mode mode;
            if( pressedDecoratorAction ) // if currently inside mouse press
            {
                mode = ( state == QIcon::On ) ? QIcon::Active : QIcon::Normal;
                const int shrinkBy = iconRect.contains( cursorPos ) && state == QIcon::On
                                   ? iconRect.width() / 16 : 0;
                iconRect.adjust( shrinkBy, shrinkBy,  -2 * shrinkBy, -2 * shrinkBy );
            }
            else
                mode = iconRect.contains( cursorPos ) ? QIcon::Active : QIcon::Normal;
            actions[i]->icon().paint( painter, iconRect, Qt::AlignCenter, mode, state );
        }
    }

    painter->setFont( m_smallFont );  // we want smaller font for both subtitle and capacity bar
    //show the bylinetext or the capacity (if available) when hovering
    if( isHover && hasCapacity )
    {
        qreal bytesUsed = index.data( PrettyTreeRoles::UsedCapacityRole ).toReal();
        qreal bytesTotal = index.data( PrettyTreeRoles::TotalCapacityRole ).toReal();
        const int percentage = (bytesTotal > 0.0) ? qRound( 100.0 * bytesUsed / bytesTotal ) : 100;

        KCapacityBar capacityBar( KCapacityBar::DrawTextInline );
        capacityBar.setValue( percentage );
        capacityBar.setText( i18nc( "Example: 3.5 GB free (unit is part of %1)", "%1 free",
                                    KFormat().formatByteSize( bytesTotal - bytesUsed ) ) );

        QRect capacityRect( remainingRect );
        capacityRect.setTop( titleRect.bottom() );
        capacityRect.setHeight( qBound( CAPACITYRECT_MIN_HEIGHT,
                                        capacityBar.minimumSizeHint().height(),
                                        qMin( CAPACITYRECT_MAX_HEIGHT, capacityRect.height() ) ) );

        capacityBar.drawCapacityBar( painter, capacityRect );
    }
    else
    {
        QRectF textRect( remainingRect );
        textRect.setTop( titleRect.bottom() );
        textRect.setHeight( remainingRect.height() - titleRect.height() );

        QString byLineText = index.data( PrettyTreeRoles::ByLineRole ).toString();
        byLineText = m_smallFm->elidedText( byLineText, Qt::ElideRight, textRect.width() );

        painter->drawText( textRect, byLineText );
    }

    painter->restore();
}

QSize
PrettyTreeDelegate::sizeHint( const QStyleOptionViewItem &option,
                              const QModelIndex &index ) const
{
    // note: the QStyledItemDelegage::sizeHint seems to be extremely slow. don't call it

    updateFonts( option );
    QStyle *style = m_view->style();

    PixelSizes s( style );
    int viewportWidth = m_view->viewport()->width();
    int normalHeight = s.frameVMargin + qMax( s.smallIconSize, m_normalFm->height() ) + s.frameVMargin;

    const bool hasCover = index.data( PrettyTreeRoles::HasCoverRole ).toBool();

    // -- determine if we have an album
    if( hasCover )
        return QSize( viewportWidth, qMax( normalHeight, s.largeIconSize + s.frameVMargin * 2 ) );

    // -- not top level. this is a normal item
    if( index.parent().isValid() )
        return QSize( viewportWidth, normalHeight );

    // -- ok, we have a top level item
    const bool hasCapacity = index.data( PrettyTreeRoles::HasCapacityRole ).toBool();

    QSize iconSize( s.largeIconSize, s.largeIconSize );
    QSize expanderSize( s.expanderIconSize, s.expanderIconSize );
    QSize titleSize( m_bigFm->boundingRect( index.data( Qt::DisplayRole ).toString() ).size() );
    QSize byLineSize( m_smallFm->boundingRect( index.data( PrettyTreeRoles::ByLineRole ).toString() ).size() );
    QSize capacitySize( hasCapacity ? 10 : 0, hasCapacity ? CAPACITYRECT_MIN_HEIGHT : 0 );

    int layoutWidth = s.frameHMargin +
        iconSize.width() + s.iconSpacing +
        qMax( titleSize.width(), byLineSize.width() ) + expanderSize.width() +
        s.frameHMargin;

    layoutWidth = qMax( viewportWidth, layoutWidth );

    int layoutHeight = s.frameVMargin + s.frameExtraMargin +
        qMax( titleSize.height(), s.smallIconSize ) + s.verticalSpace +
        qMax( capacitySize.height(), byLineSize.height() ) +
        s.frameExtraMargin + s.frameVMargin;

    return QSize( layoutWidth, layoutHeight );
}

QRect
PrettyTreeDelegate::decoratorRect( const QRect &itemRect, int nr ) const
{
    PixelSizes s( m_view->style() );
    const bool isRTL = QApplication::isRightToLeft();

    int y = itemRect.top() + s.frameVMargin + s.frameExtraMargin;
    int xOffset = s.frameHMargin + s.expanderIconSize + s.iconSpacing + ( s.smallIconSize + s.iconSpacing ) * nr;
    int x;

    if( isRTL )
        x = itemRect.left() + xOffset;
    else
        x = itemRect.right() - xOffset - s.smallIconSize;

    return QRect( QPoint( x, y ), QSize( s.smallIconSize, s.smallIconSize ) );
}

void
PrettyTreeDelegate::updateFonts( const QStyleOptionViewItem &option ) const
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
