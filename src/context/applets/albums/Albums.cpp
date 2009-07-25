/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2009 simon.esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "Albums.h"

#include "AlbumItem.h"
#include "AlbumsView.h"
#include "AlbumsModel.h"
#include "Amarok.h"
#include "collection/Collection.h"
#include "collection/CollectionManager.h"
#include "Debug.h"
#include "EngineController.h"
#include "context/Svg.h"
#include "context/widgets/TextScrollingWidget.h"
#include "meta/Meta.h"
#include "meta/MetaUtility.h"
#include "playlist/PlaylistModel.h"
#include "TrackItem.h"

#include <plasma/theme.h>


#include <QPainter>
#include <QTreeView>

Albums::Albums( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_configLayout( 0 )
    , m_width( 0 )
    , m_albumWidth( 50 )
{
    setHasConfigurationInterface( false );
}

Albums::~Albums()
{
    delete m_albumsView->widget();
}

void Albums::init()
{
    setBackgroundHints( Plasma::Applet::NoBackground );
    
    m_headerText = new TextScrollingWidget( this );

    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerText->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerText->setFont( labelFont );
    m_headerText->setText( i18n( "Recently added albums" ) );
    
    m_width = globalConfig().readEntry( "width", 500 );
    m_height = globalConfig().readEntry( "height", 300 );

    m_albumsView = new AlbumsView( this );
    m_albumsView->setMinimumSize( 100, 150 );
    
    m_model = new AlbumsModel();
    m_model->setColumnCount( 1 );
    m_albumsView->setModel( m_model );
    m_albumsView->show();

    // properly set the height
    resize( m_width, m_height );

    dataEngine( "amarok-current" )->connectSource( "albums", this );

    connect( dataEngine( "amarok-current" ), SIGNAL( sourceAdded( const QString& ) ),
             this, SLOT( connectSource( const QString& ) ) );

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
//    DEBUG_BLOCK

    qreal widmax = boundingRect().width() - 4 * standardPadding();
    QRectF rect( ( boundingRect().width() - widmax ) / 2, 0 , widmax, 15 );

    m_headerText->setScrollingText( m_headerText->text(), rect );

    // here we put all of the text items into the correct locations
    m_headerText->setPos( ( size().width() - m_headerText->boundingRect().width() ) / 2 , standardPadding() + 3 );
    
 //   debug() << "Updating constraints for " << m_albumCount << " album rows";
    m_albumsView->resize( size().toSize().width() - 2 * standardPadding() , size().toSize().height() - m_headerText->boundingRect().height() - 3 * standardPadding()  );
    m_albumsView->setPos( standardPadding(), m_headerText->pos().y() + m_headerText->boundingRect().height() + standardPadding() );

}

void Albums::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );
   
    m_albums = data[ "albums" ].value<Meta::AlbumList>();
    if( m_albums.isEmpty() )
        return;

    debug() << "Received" << m_albums.count() << "albums";

    m_headerText->setText( data[ "headerText" ].toString() );

    //Update the applet (render properly the header)
    update();

    m_model->clear();
       
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    Meta::AlbumPtr currentAlbum;

    // Here's a smallish hack to sort the albums based on year:
    // Put them into a QMultiMap with the key as the year, and then retrieve the QList. Tada!
    // A little memory inefficient but much more convenient than qSort with Meta::AlbumPtrs.
    // We only want to sort if we have a current track playing, otherwise we mess up the "recent tracks"
    if( currentTrack )
    {
        currentAlbum = currentTrack->album();
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

    const bool showArtist = !currentTrack;

    foreach( Meta::AlbumPtr albumPtr, m_albums )
    {
        AlbumItem *albumItem = new AlbumItem();
        albumItem->setIconSize( m_albumWidth );
        albumItem->setAlbum( albumPtr );
        albumItem->setShowArtist( showArtist );
        
        int discNumber = 0;

        int childRow = 0;
        foreach( Meta::TrackPtr trackPtr, albumPtr->tracks() )
        {
            if( trackPtr->discNumber() != discNumber )
            {
                discNumber = trackPtr->discNumber();
                QStandardItem *disc = new QStandardItem();
                disc->setText( i18n( "Disc %1", discNumber ) );
                albumItem->setChild( childRow++, disc );
            }
            TrackItem *trackItem = new TrackItem();
            trackItem->setTrack( trackPtr );
            
            // Italicise the current track to make it more visible
            if( currentTrack == trackPtr )
                trackItem->italicise();

            // If compilation and same artist, then make bold, but only if there's a current track
            if( currentTrack && currentTrack->artist() == trackPtr->artist() && albumPtr->isCompilation() )
                trackItem->bold();           
            
            albumItem->setChild( childRow++, trackItem );
        }
        
        m_model->appendRow( albumItem );
        if( currentAlbum && currentAlbum == albumPtr )
           m_albumsView->nativeWidget()->expand( m_model->indexFromItem( albumItem ) );
    }
    
    updateConstraints();
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


    p->setRenderHint( QPainter::Antialiasing );

    // tint the whole applet
    addGradientToAppletBackground( p );

    // draw rounded rect around title if not currently animating
    if ( !m_headerText->isAnimating() )
        drawRoundedRectAroundText( p, m_headerText );

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

#include "Albums.moc"

