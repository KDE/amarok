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
#include "Amarok.h"
#include "collection/Collection.h"
#include "collection/CollectionManager.h"
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
{
    setHasConfigurationInterface( false );
}

Albums::~Albums()
{}

void Albums::init()
{
    setBackgroundHints( Plasma::Applet::NoBackground );
    
    m_headerText = new QGraphicsSimpleTextItem( this );
    m_headerText->setText( i18n( "Recently added albums" ) );
    m_width = globalConfig().readEntry( "width", 500 );
    m_height = globalConfig().readEntry( "height", 300 );

    m_albumsView = new AlbumsView( this );
    m_albumsView->setMinimumSize( 100, 150 );
    
    m_model = new QStandardItemModel();
    m_model->setColumnCount( 1 );
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
    connect( m_albumsView, SIGNAL( enqueueTrack( const QString &, const QString & ) ),
             this, SLOT( enqueueTrack( const QString &, const QString & ) ) );
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

    // here we put all of the text items into the correct locations
    m_headerText->setPos( size().width() / 2 - m_headerText->boundingRect().width() / 2, margin );
    
    debug() << "Updating constraints for " << m_albumCount << " album rows";
    m_albumsView->resize( size().toSize().width() - margin , size().toSize().height() - margin * 4 );

}

void Albums::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );
   
    m_albums = data[ "albums" ].value<Meta::AlbumList>();
    if( m_albums.isEmpty() )
        return;

    m_headerText->setText( data[ "headerText" ].toString() );
    
    m_model->clear();
       
    int row = 0;

    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();

    // Here's a smallish hack to sort the albums based on year:
    // Put them into a QMultiMap with the key as the year, and then retrieve the QList. Tada!
    // A little memory inefficient but much more convenient than qSort with Meta::AlbumPtrs.
    // We only want to sort if we have a current track playing, otherwise we mess up the "recent tracks"
    if( currentTrack )
    {
        QMultiMap<QString, Meta::AlbumPtr> map; // MultiMap, as we can have multiple albums with the same year
        foreach( Meta::AlbumPtr albumPtr, m_albums )
        {
            int year = 0;
            if( !albumPtr->tracks().isEmpty() )
                year = albumPtr->tracks().first()->year()->name().toInt();
            // Here's another little hack. Because we want the albums in reverse chronological order,
            // we can simply store a larger number for the earlier albums.
            year = 9999 - year;

            map.insert( QString::number(year), albumPtr );
        }
        m_albums = map.values();
    }

    foreach( Meta::AlbumPtr albumPtr, m_albums )
    {
        QStandardItem *albumItem = new QStandardItem();

        QString albumName = albumPtr->name();
        albumName = albumName.isEmpty() ? i18n("Unknown") : albumName;

        QString displayText = albumName;

        Meta::TrackList tracks = albumPtr->tracks();
        QString year;
        if( !tracks.isEmpty() )
        {
            Meta::TrackPtr first = tracks.first();
            year = first->year()->name();
            // do some sanity checking
            if( year.length() != 4 )
                year = QString();
        }

        if( !year.isEmpty() )
            displayText += QString( " (%1)" ).arg( year );

        QString trackCount = i18np( "%1 track", "%1 tracks", albumPtr->tracks().size() );
        displayText += "\n" + trackCount;

        albumItem->setText( displayText );

        QPixmap cover = albumPtr->image( m_albumWidth );
        albumItem->setIcon( QIcon( cover ) );
        
        QSize sizeHint = albumItem->sizeHint();
        sizeHint.setHeight( 80 );
        albumItem->setSizeHint( sizeHint );
       
        int childRow = 0;
        foreach( Meta::TrackPtr trackPtr, albumPtr->tracks() )
        {
            int trackNumber = trackPtr->trackNumber();

            QString text;

            if( trackNumber > 0 )
                text = QString( "%1 - %2" ).arg( QString::number(trackPtr->trackNumber()), trackPtr->prettyName() );
            else
                text = trackPtr->prettyName();

            QStandardItem *trackItem = new QStandardItem();
            trackItem->setText( text );
            
            // Italicise the current track to make it more visible
            if( currentTrack == trackPtr )
            {
                QFont f = trackItem->font();
                f.setItalic( true );
                trackItem->setFont( f );
            }

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
    Collection *coll = CollectionManager::instance()->primaryCollection();
    m_qm = coll->queryMaker();
    m_qm->setQueryType( QueryMaker::Album );
    m_qm->addFilter( QueryMaker::valAlbum, name );

    connect( m_qm, SIGNAL( newResultReady( QString, Meta::AlbumList ) ),
             this,   SLOT(    resultReady( QString, Meta::AlbumList ) ),
             Qt::QueuedConnection );
    
    m_qm->run();

}

void
Albums::enqueueTrack( const QString &albumName, const QString &trackName )
{
    DEBUG_BLOCK
    Collection *coll = CollectionManager::instance()->primaryCollection();
    m_qm = coll->queryMaker();
    m_qm->setQueryType( QueryMaker::Track );
    
    m_qm->addFilter( QueryMaker::valAlbum, albumName );
    m_qm->addFilter( QueryMaker::valTitle, trackName );
    
    connect( m_qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ),
             this,   SLOT(    resultReady( QString, Meta::TrackList ) ),
             Qt::QueuedConnection );

    m_qm->run();
}

void
Albums::resultReady( const QString &collectionId, const Meta::AlbumList &albumList )
{
    Q_UNUSED( collectionId )
    foreach( Meta::AlbumPtr albumPtr, albumList )
        The::playlistModel()->insertOptioned( albumPtr->tracks(), Playlist::Append );
}

void
Albums::resultReady( const QString &collectionId, const Meta::TrackList &trackList )
{
    DEBUG_BLOCK
    Q_UNUSED( collectionId )
    foreach( Meta::TrackPtr trackPtr, trackList )
        The::playlistModel()->insertOptioned( trackPtr, Playlist::Append );
}

#include "Albums.moc"
