/*******************************************************************************
* copyright              : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
*                                                                              *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/

#include "AlbumsDelegate.h"
#include "Debug.h"

#include <plasma/paintutils.h>
#include <plasma/theme.h>

#include <KColorUtils>
#include <KGlobal>
#include <KGlobalSettings>
#include <KColorScheme>

#include <QAction>
#include <QEvent>
#include <QFontMetrics>
#include <QIcon>
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QToolButton>



AlbumsDelegate::AlbumsDelegate( QObject *parent )
    : QAbstractItemDelegate( parent )
    , m_coverWidth( 60 )
{
    m_decorations = new Context::Svg( this );
    m_decorations->setImagePath( "widgets/amarok-albums" );
    m_decorations->setContainsMultipleImages( true );
    m_coverIcon = KIcon( "media-album-cover" );
    m_trackIcon = KIcon( "media-album-track" );
}

AlbumsDelegate::~AlbumsDelegate()
{
}


void
AlbumsDelegate::paint( QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    const int radius = 5;

    QColor backgroundColor = KColorScheme(QPalette::Active, KColorScheme::Window,
                               Plasma::Theme::defaultTheme()->colorScheme()).background().color();
    
    QRect backgroundRect( option.rect );
    backgroundRect.adjust( 5, 5, 5, 0 );

    painter->save();
    if( m_highlightedRow == index )
    {
        painter->setOpacity( 0.8 );
        backgroundColor.setAlpha( 50 );
    }
    else
    {
        painter->setOpacity( 0.3 );
        backgroundColor.setAlpha( 30 );
    }
    painter->fillRect( backgroundRect, backgroundColor );
    painter->restore();
    
    QPainterPath path;

    path = Plasma::PaintUtils::roundedRectangle( backgroundRect.adjusted( 0, 0, radius, 0 ), radius );

    painter->save();

    painter->setClipRect( backgroundRect );
    painter->setRenderHint( QPainter::Antialiasing );
    painter->setPen( Qt::NoPen );
    painter->setBrush( backgroundColor );
    painter->drawPath( path );

    painter->restore();    

    const qreal margin = 14.0;
    const qreal labelX = m_coverWidth + margin;

    const qreal iconX = labelX + margin;

    QString titleText;
    QFontMetrics fm( painter->font() );
    if( index.data( AlbumRoles::TrackName ) != QVariant() )
    {
        QRect titleRect( QPoint( option.rect.left() + margin + 22, option.rect.top() + option.rect.height()/4  ),
                     QSize( option.rect.width() - ( margin + 22 ), option.rect.height() ) );
                     
        titleText = fm.elidedText( QString( "%1 - " ).arg( index.row() + 1 ) + index.data( AlbumRoles::TrackName ).value<QString>(), Qt::ElideRight, option.rect.width() * 0.6 );

        m_trackIcon.paint( painter, QRect( QPoint( option.rect.left() + margin,
                                                 option.rect.top() + option.rect.height()/4 + 4 ),                                         
                                         QSize( 16, 16 ) ) );
        painter->save();
        painter->setPen( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
        painter->drawText( titleRect, Qt::AlignLeft, titleText );
        painter->restore();
    }
    else
    {
        QRect titleRect( QPoint( option.rect.left() + iconX + 20, option.rect.top() + margin + 1 ),
                     QSize( option.rect.width() - ( iconX + 20 ), option.rect.height() ) );
        QRect trackCountRect( QPoint( option.rect.left() + iconX + 20, option.rect.top() + ( margin + 6 ) * 2  ),
                     QSize( option.rect.width() - ( iconX + 20 ), option.rect.height() ) );
                     
        titleText = fm.elidedText( index.data( AlbumRoles::AlbumName ).value<QString>(), Qt::ElideRight, option.rect.width() * 0.6 );
        QString trackText = index.data( AlbumRoles::TrackCount ).value<QString>();
      
        painter->save();
        painter->setPen( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
        painter->drawText( titleRect, Qt::AlignLeft, titleText );
        painter->drawText( trackCountRect, Qt::AlignLeft, trackText );
        painter->restore();        
        
        m_coverIcon.paint( painter, QRect( QPoint( option.rect.left() + iconX, option.rect.top() + margin + 5 ),
                                         QSize( 16, 16 ) ) );
        
        m_trackIcon.paint( painter, QRect( QPoint( option.rect.left() + iconX, option.rect.top() + ( margin + 8 )* 2 ),
                                         QSize( 16, 16 ) ) );

        QPoint coverTopLeft( option.rect.left() + margin, option.rect.top() + margin );
        QSize  coverSize( m_coverWidth, m_coverWidth );
        QRect albumCoverRect( coverTopLeft, coverSize );

        QPixmap coverPixmap = index.data( AlbumRoles::AlbumCover ).value<QPixmap>();

        painter->drawPixmap( albumCoverRect, coverPixmap );
    }

}

QSize
AlbumsDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size = option.rect.size();
    
    QFontMetrics metrics( option.font );
    
    if( index.parent() == QModelIndex() )
    {
        size.setHeight( 80 );
    }
    else
    {
        size.setHeight( metrics.height()* 1.5 );
    }
    
    size.setWidth( option.rect.width() );
    return size;   
}

void
AlbumsDelegate::highlightRow( const QModelIndex &index )
{
    m_highlightedRow = index;
    QAbstractItemDelegate::event( new QEvent( QEvent::Paint ) );
}

