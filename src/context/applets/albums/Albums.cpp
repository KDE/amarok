/****************************************************************************
 * copyright            : (C) 2008 Seb Ruiz <ruiz@kde.org>                  *
 *                        (C) 2008 William Viana Soares <vianasw@gmail.com> *
 ****************************************************************************/

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
#include "playlist/PlaylistModel.h"

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
    , m_albumCount( 0 )
{
    setHasConfigurationInterface( false );
    prepareElements();
}

Albums::~Albums()
{}

void Albums::init()
{
    setBackgroundHints( Plasma::Applet::NoBackground );
 
    m_width = globalConfig().readEntry( "width", 500 );
    m_height = globalConfig().readEntry( "height", 300 );

    // get natural aspect ratio, so we can keep it on resize
    m_aspectRatio = m_width / m_height;
    resize( m_width, m_height );

    dataEngine( "amarok-current" )->connectSource( "albums", this );

    connect( dataEngine( "amarok-current" ), SIGNAL( sourceAdded( const QString& ) ),
             this, SLOT( connectSource( const QString& ) ) );

    updateConstraints();
}

void Albums::prepareElements()
{
    DEBUG_BLOCK


    qDeleteAll( m_albums );
    m_albums.clear();

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

    debug() << "Going to add " << m_albumCount << " elements";

    for ( int i = 0; i < m_albumCount; i++ )
    {
        debug() << i;
        QString albumName = m_names[i].toString();
        QString trackCount = m_trackCounts[i].toString();
        QPixmap image = m_covers[i].value<QPixmap>();

        AlbumEntry *album = new AlbumEntry( this );
        album->resize( QSizeF( boundingRect().width() - 10, 70 ) );

        if( albumName.isEmpty() )
            album->setAlbumName( truncateTextToFit( i18n("Unknown"), album->font(), QRectF( 0, 0, album->size().width() - 121, album->size().height() ) ) );
        else
            album->setAlbumName( truncateTextToFit( albumName, album->font(), QRectF( 0, 0, album->size().width() - 121, album->size().height() ) ) );
        //album->setAlbumName( albumName.isEmpty() ? i18n("Unknown") : albumName );

        album->setCoverImage( image );
        album->setTrackCount( trackCount );

        connect( album, SIGNAL( clicked( const QString& ) ), this, SLOT( enqueueAlbum( const QString& ) ) );
        m_albums.append( album );

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
    DEBUG_BLOCK

    //bah! do away with trying to get postions from an svg as this is proving wildly inaccurate
    const qreal margin = 14.0;

    const qreal labelX = m_albumWidth + margin + 14.0;
    const qreal labelWidth = 15;
    const qreal textX = labelX + labelWidth + margin;

    const qreal textHeight = 22;
    const qreal textWidth = size().toSize().width() - ( textX + margin );

    // here we put all of the text items into the correct locations

    debug() << "Updating constraints for " << m_albumCount << " album rows";
    for( int i = 0; i < m_albumCount; ++i )
    {
        
        AlbumEntry *album = m_albums.at( i );

        const qreal yPos = i * ( album->size().height() + margin/2 ) + margin;

        album->setPos( QPointF( margin - 10, yPos ) );
        album->resize( QSizeF( boundingRect().width() - 10, 70 ) );

    }

}

void Albums::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );

    m_albumCount = data[ "count" ].toInt();
    m_names = data[ "names" ].toList();
    m_trackCounts = data[ "trackCounts" ].toList();;
    m_covers = data[ "covers" ].toList();;

    kDebug() << "Albums::dataUpdated. count: " << m_albumCount << " names " << m_names.count();

    prepareElements();


    const qreal margin = 14;
    const qreal height = m_albums.size() * ( m_albumWidth + margin ) + margin;
    resize( size().toSize().width(), height );

    updateConstraints();
}


QSizeF 
Albums::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    Q_UNUSED( which )

    if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
    {
        return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );
    }
    else
    {
        return constraint;
    }
    
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
        
    const qreal margin = 14.0;
    const qreal labelX = m_albumWidth + margin;
    const qreal textHeight = 22;
    
    const qreal iconX = labelX + margin;

    for( int i = 0; i < m_albums.size(); ++i )
    {
        m_albums.at( i )->show();
    }
    p->restore();
}

void Albums::showConfigurationInterface()
{}

void Albums::configAccepted() // SLOT
{}

void Albums::connectSource( const QString &source )
{
    if( source == "albums" )
    {
        dataEngine( "amarok-current" )->connectSource( source, this );
        dataUpdated( source, dataEngine("amarok-current" )->query( "albums" ) ); // get data initally
    }
}

void Albums::enqueueAlbum( const QString &name )
{
    foreach( Meta::AlbumPtr aptr, The::engineController()->currentTrack()->artist()->albums() )
    {
        if( aptr->name() == name )
            The::playlistModel()->insertOptioned( aptr->tracks(), Playlist::Append );
    }
}

AlbumTextItem::AlbumTextItem( QGraphicsItem * parent )
    : QGraphicsSimpleTextItem( parent )
{
}

void AlbumTextItem::mousePressEvent( QGraphicsSceneMouseEvent * event )
{
    Q_UNUSED( event )
    emit( clicked( text() ) );
}

#include "Albums.moc"
