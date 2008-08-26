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
#include "AlbumsView.h"
#include "AlbumsDelegate.h"
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
}

Albums::~Albums()
{}

void Albums::init()
{
    setBackgroundHints( Plasma::Applet::NoBackground );

    m_headerText = new QGraphicsSimpleTextItem( this );    
 
    m_width = globalConfig().readEntry( "width", 500 );
    m_height = globalConfig().readEntry( "height", 300 );

    m_albumsView = new AlbumsView( this );
    m_albumsView->setMinimumSize( 100, 150 );
    
    m_model = new QStandardItemModel();
    m_albumsView->setModel( m_model );
    m_albumsView->resize( size().width() - 28, size().height() - 28 );
    m_albumsView->setPos( 7, 42 );
    m_albumsView->show();
    // get natural aspect ratio, so we can keep it on resize
    m_aspectRatio = m_width / m_height;
    resize( m_width, m_height );

    dataEngine( "amarok-current" )->connectSource( "albums", this );

    connect( dataEngine( "amarok-current" ), SIGNAL( sourceAdded( const QString& ) ),
             this, SLOT( connectSource( const QString& ) ) );

    connect( m_albumsView, SIGNAL( enqueueAlbum( const QString & ) ), this, SLOT( enqueueAlbum( const QString & ) ) );
    connect( m_albumsView, SIGNAL( enqueueTrack( const QString & ) ), this, SLOT( enqueueTrack( const QString & ) ) );
    updateConstraints();
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

    // here we put all of the text items into the correct locations
    m_headerText->setPos( size().width() / 2 - m_headerText->boundingRect().width() / 2, margin );
    
    debug() << "Updating constraints for " << m_albumCount << " album rows";
    m_albumsView->resize( size().toSize().width() - margin , size().toSize().height() - margin * 4 );

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

    m_model->clear();
    m_model->setColumnCount( 1 );
    int row = 0;
    QString artistName = The::engineController()->currentTrack()->artist()->name();
    m_headerText->setText( i18n( "Albums by %1", artistName ) );
    foreach( Meta::AlbumPtr aptr, The::engineController()->currentTrack()->artist()->albums() )
    {
        QString albumName = aptr->name();
        
        QStandardItem *albumItem = new QStandardItem();
        albumItem->setData( albumName, AlbumRoles::AlbumName );
        
        if( m_trackCounts.size() > 0 )
            albumItem->setData( m_trackCounts[row].toString(), AlbumRoles::TrackCount );

        if( m_covers.size() > 0 )
            albumItem->setData( m_covers[row].value<QPixmap>(), AlbumRoles::AlbumCover );
        
        int childRow = 0;
        foreach( Meta::TrackPtr track, aptr->tracks() )
        {
            QStandardItem *trackItem = new QStandardItem();
            trackItem->setData( track->name(), AlbumRoles::TrackName );
            albumItem->setChild( childRow, trackItem );
            childRow++;
        }
        
        m_model->appendRow( albumItem );
        row++;
    }
    
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
    Q_UNUSED( p )
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

void
Albums::enqueueTrack( const QString &name )
{
    foreach( Meta::TrackPtr tptr, The::engineController()->currentTrack()->artist()->tracks() )
    {
        if( tptr->name() == name )
            The::playlistModel()->insertOptioned( tptr, Playlist::Append );
    }
}


#include "Albums.moc"
