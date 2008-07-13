/***************************************************************************
 * copyright            : (C) 2008 Seb Ruiz <ruiz@kde.org>                 *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "Albums.h"

#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h"
#include "context/Svg.h"
#include "context/popupdropper/PopupDropperAction.h"
#include "meta/Meta.h"
#include "meta/MetaUtility.h"

#include <plasma/theme.h>

#include <KApplication>
#include <KIcon>
#include <KMessageBox>

#include <QPainter>
#include <QBrush>
#include <QFont>
#include <QVBoxLayout>
#include <QLabel>
#include <QMap>

Albums::Albums( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_configLayout( 0 )
    , m_width( 0 )
    , m_albumWidth( 50 )
    , m_aspectRatio( 0.0 )
{
    setHasConfigurationInterface( false );
    prepareElements();
}

Albums::~Albums()
{}

void Albums::init()
{
    setBackgroundHints( Plasma::Applet::NoBackground );

    m_theme = new Context::Svg( this );
    m_theme->setImagePath( "widgets/amarok-albums" );
    m_theme->setContainsMultipleImages( true );
 
    m_width = globalConfig().readEntry( "width", 500 );
    
    m_artistLabel = new QGraphicsSimpleTextItem( this );

    // get natural aspect ratio, so we can keep it on resize
    m_theme->resize();
    m_aspectRatio = (qreal)m_theme->size().height() / (qreal)m_theme->size().width();
    resize( m_width, m_aspectRatio );

    dataEngine( "amarok-albums" )->connectSource( "albums", this );
}

void Albums::prepareElements()
{
    DEBUG_BLOCK

    m_albumLabels.clear();
    m_albumCovers.clear();
    m_albumTracks.clear();

    const QColor textColor( Qt::white );
    QFont labelFont;
    labelFont.setBold( true );
    labelFont.setPointSize( labelFont.pointSize() + 1  );
    labelFont.setStyleHint( QFont::Times );
    labelFont.setStyleStrategy( QFont::PreferAntialias );

    QFont textFont = QFont( labelFont );
    textFont.setBold( false );

    QFont tinyFont( textFont );
    tinyFont.setPointSize( tinyFont.pointSize() - 5 );
    tinyFont.setBold( true );
    
    Meta::TrackPtr  track  = The::engineController()->currentTrack();
    Meta::ArtistPtr artist = track->artist();
    Meta::AlbumList albums = artist->albums();

    debug() << "current track: " << track->name();
    debug() << "current artist: " << artist->name();
    debug() << "artist albums: " << albums.size();

    foreach( Meta::AlbumPtr albumPtr, albums )
    {
        debug() << "Creating elements for " << albumPtr->name();
        QGraphicsSimpleTextItem *album = new QGraphicsSimpleTextItem( this );
        QGraphicsSimpleTextItem *trackCount = new QGraphicsSimpleTextItem( this );
        QGraphicsPixmapItem     *cover = new QGraphicsPixmapItem( this );
        
        album->setText( albumPtr->name() );
        album->setFont( textFont );
        album->setBrush( textColor );

        trackCount->setText( i18np( "%1 track", "%1 tracks", albumPtr->tracks().size() ) );
        trackCount->setFont( textFont );
        trackCount->setBrush( textColor );

        QPixmap image = albumPtr->image( m_albumWidth );
        cover->setPixmap( image );

        m_albumLabels.append( album );
        m_albumCovers.append( cover );
        m_albumTracks.append( trackCount );
    }
}

QList<QAction*>
Albums::contextualActions()
{
    QList<QAction*> actions;
    return actions;
}

void Albums::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )

    prepareGeometryChange();

    //bah! do away with trying to get postions from an svg as this is proving wildly inaccurate
    const qreal margin = 14.0;

    const qreal labelX = m_albumWidth + margin + 14.0;
    const qreal labelWidth = 15;
    const qreal textX = labelX + labelWidth + margin;

    const qreal textHeight = 22;
    const qreal textWidth = size().toSize().width() - ( textX + margin );

    // here we put all of the text items into the correct locations

    debug() << "Updating constraints for " << m_albumLabels.size() << " album rows";
    for( int i = 0; i < m_albumLabels.size(); ++i )
    {
        QGraphicsSimpleTextItem *album      = m_albumLabels.at( i );
        QGraphicsSimpleTextItem *trackCount = m_albumTracks.at( i );
        QGraphicsPixmapItem     *cover      = m_albumCovers.at( i );

        const qreal yPos = i * ( m_albumWidth + margin ) + margin;

        album->setPos( QPointF( textX, yPos ) );
        cover->setPos( QPointF( margin + 2, yPos ) );
        trackCount->setPos( QPointF( textX, yPos + textHeight ) );

        QString albumText = album->text();
        debug() << "   --> " << albumText << " " << album->pos();

        QFont textFont = shrinkTextSizeToFit( albumText, QRectF( 0, 0, textWidth, textHeight ) );
        QFont tinyFont( textFont );

        if( tinyFont.pointSize() > 5 ) tinyFont.setPointSize( tinyFont.pointSize() - 5 );
        else                           tinyFont.setPointSize( 1 );
    
        tinyFont.setBold( true );
    
        m_maxTextWidth = size().toSize().width() - album->pos().x() - 14;

        const QRectF rect = QRectF( 0, 0, textWidth, 30 );

        album->setFont( textFont );
        album->setText( truncateTextToFit( albumText, album->font(), rect ) );

        QString trackText = trackCount->text();
        trackCount->setFont( textFont );
        trackCount->setText( truncateTextToFit( trackText, trackCount->font(), rect ) );
    }
}

void Albums::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );
    Q_UNUSED( data );

    kDebug() << "Albums::dataUpdated";

    prepareElements();

    update();
}


QSizeF 
Albums::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    Q_UNUSED( which )

    if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
    {
        //return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );
        const qreal margin = 14;
        const qreal height = m_albumLabels.size() * ( m_albumWidth + margin ) + margin;
        return QSizeF( constraint.width(), height );
    }

    return constraint;
}

void Albums::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );

    //bail out if there is no room to paint. Prevents crashes and really there is no sense in painting if the
    //context view has been minimized completely
    if( ( contentsRect.width() < 20 ) || ( contentsRect.height() < 20 ) )
    {
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children() )
            childItem->hide();
        return;
    }
    else
    {
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children () )
            childItem->show();
    }

    p->save();
    m_theme->paint( p, contentsRect.adjusted( 0, -10, 0, 10 ) , "background" );
    QRect leftBorder( 0, 0, 14, contentsRect.height() + 20 );
    m_theme->paint( p, leftBorder, "left-border" );
    QRect rightBorder( contentsRect.width() + 5, 0, 14, contentsRect.height() + 20 );
    m_theme->paint( p, rightBorder, "right-border" );
    p->restore();

    p->save();
    
    
    //m_theme->paint( p, QRectF( labelX, currentY, 16, 16 ), "artist" );
    
    const qreal margin = 14.0;
    const qreal labelX = m_albumWidth + margin;
    const qreal textHeight = 22;
    
    const qreal iconX = labelX + margin;

    for( int i = 0; i < m_albumLabels.size(); ++i )
    {
        const qreal yPos = i * ( m_albumWidth + margin ) + margin;

        m_theme->paint( p, QRect( margin - 5, yPos - 1, m_albumWidth + 12, m_albumWidth + 2), "cd-box" );
        m_theme->paint( p, QRectF( iconX, yPos, 16, 16 ), "album" );
        m_theme->paint( p, QRectF( iconX, yPos + textHeight, 16, 16 ), "track" );
    }
    p->restore();
}

void Albums::showConfigurationInterface()
{}

void Albums::configAccepted() // SLOT
{}

#include "Albums.moc"
