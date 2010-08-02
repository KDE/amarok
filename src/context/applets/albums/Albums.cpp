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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "Albums.h"

#include "AlbumItem.h"
#include "AlbumsView.h"
#include "AlbumsModel.h"
#include "core/support/Amarok.h"
#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "context/Svg.h"
#include "context/widgets/TextScrollingWidget.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "playlist/PlaylistModel.h"
#include "TrackItem.h"

#include <plasma/theme.h>
#include <KConfigDialog>

#include <QFormLayout>
#include <QPainter>
#include <QSpinBox>
#include <QTreeView>
#include <QGraphicsLinearLayout>

Albums::Albums( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_recentCount( Amarok::config("Albums Applet").readEntry("RecentlyAdded", 5) )
    , m_albumsView( 0 )
{
    setHasConfigurationInterface( true );
}

Albums::~Albums()
{
}

void Albums::init()
{
    // Call the base implementation.
    Context::Applet::init();

    setBackgroundHints( Plasma::Applet::NoBackground );

    // properly set the height
    // -1 means ask for all available space left
    resize( globalConfig().readEntry( "width", 500 ), -1 );

    m_headerText = new TextScrollingWidget( this );
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerText->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerText->setFont( labelFont );
    m_headerText->setText( i18n( "Recently added albums" ) );

    QGraphicsLinearLayout *headerLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    headerLayout->setContentsMargins( 0, 4, 0, 2 );
    headerLayout->addItem( m_headerText );

    m_albumsView = new AlbumsView( this );
    m_albumsView->setMinimumSize( 100, 150 );
    m_model = new AlbumsModel( this );
    m_model->setColumnCount( 1 );
    m_albumsView->setModel( m_model );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical );
    layout->addItem( headerLayout );
    layout->addItem( m_albumsView );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    setLayout( layout );

    dataEngine( "amarok-current" )->connectSource( "albums", this );
    connect( dataEngine( "amarok-current" ), SIGNAL(sourceAdded(QString)),
             this, SLOT(connectSource(QString)) );
    connect( CollectionManager::instance(), SIGNAL(collectionDataChanged(Collections::Collection*)),
             this, SLOT(collectionDataChanged(Collections::Collection*)) );

    updateConstraints();
}

void Albums::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )
    m_headerText->setScrollingText( m_headerText->text() );
}

void Albums::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name );

    m_albumsView->clear();
    Meta::AlbumList albums = data[ "albums" ].value<Meta::AlbumList>();
    m_headerText->setScrollingText( data[ "headerText" ].toString() );

    //Update the applet (render properly the header)
    update();

    //Don't keep showing the albums for the artist of the last track that had album in the collection
    if( albums.isEmpty() )
        return;
       
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    Meta::AlbumPtr currentAlbum;

    const bool showArtist = !currentTrack;
    AlbumItem *scrollToAlbum( 0 );

    foreach( Meta::AlbumPtr albumPtr, albums )
    {
        AlbumItem *albumItem = new AlbumItem();
        albumItem->setIconSize( 50 );
        albumItem->setAlbum( albumPtr );
        albumItem->setShowArtist( showArtist );
        
        int numberOfDiscs = 0;
        int childRow = 0;

        Meta::TrackList tracks = albumPtr->tracks();
        qStableSort( tracks.begin(), tracks.end(), Meta::Track::lessThan );

        QMultiHash< int, TrackItem* > trackItems; // hash of tracks items for each disc
        foreach( Meta::TrackPtr trackPtr, tracks )
        {
            if( numberOfDiscs < trackPtr->discNumber() )
                numberOfDiscs = trackPtr->discNumber();

            TrackItem *trackItem = new TrackItem();
            trackItem->setTrack( trackPtr );
            
            // Italicise the current track to make it more visible
            if( currentTrack == trackPtr )
                trackItem->italicise();

            // If compilation and same artist, then make bold, but only if there's a current track
            if( currentTrack && currentTrack->artist() == trackPtr->artist() && albumPtr->isCompilation() )
                trackItem->bold();           

            trackItems.insert( trackPtr->discNumber(), trackItem );
        }

        for( int i = 0; i <= numberOfDiscs; ++i )
        {
            QList<TrackItem*> items = trackItems.values( i );
            if( !items.isEmpty() )
            {
                const TrackItem *item = items.first();
                QStandardItem *discItem( 0 );
                if( numberOfDiscs > 0 )
                {
                    discItem = new QStandardItem( i18n("Disc %1", item->track()->discNumber()) );
                    albumItem->setChild( childRow++, discItem );
                    int discChildRow = 0;
                    foreach( TrackItem *trackItem, items )
                        discItem->setChild( discChildRow++, trackItem );
                }
                else
                {
                    foreach( TrackItem *trackItem, items )
                        albumItem->setChild( childRow++, trackItem );
                }
            }
        }
        
        m_model->appendRow( albumItem );
        if( currentAlbum && currentAlbum == albumPtr )
        {
            m_albumsView->setRecursiveExpanded( albumItem->index(), true );
            scrollToAlbum = albumItem;
        }
    }

    if( scrollToAlbum )
        m_albumsView->nativeWidget()->scrollTo( scrollToAlbum->index(), QAbstractItemView::PositionAtTop );

    m_model->sort( 0 );
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

void Albums::createConfigurationInterface( KConfigDialog *parent )
{
    QSpinBox *spinBox = new QSpinBox;
    spinBox->setRange( 1, 100 );
    spinBox->setValue( m_recentCount );
    connect( spinBox, SIGNAL(valueChanged(int)), SLOT(setRecentCount(int)) );

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow( i18n("Number of recently added albums:"), spinBox );

    QWidget *config = new QWidget;
    config->setLayout( formLayout );

    parent->addPage( config, i18n( "Albums Applet Settings" ), "preferences-system");
    connect( parent, SIGNAL(accepted()), this, SLOT(saveConfiguration()) );
}

void Albums::setRecentCount( int val )
{
    m_recentCount = val;
}

void Albums::connectSource( const QString &source )
{
    if( source == "albums" )
    {
        dataEngine( "amarok-current" )->connectSource( source, this );
        dataUpdated( source, dataEngine("amarok-current" )->query( "albums" ) ); // get data initially
    }
}

void Albums::saveConfiguration()
{
    Amarok::config("Albums Applet").writeEntry( "RecentlyAdded", QString::number( m_recentCount ) );
    reconnectSource();
}

void Albums::collectionDataChanged( Collections::Collection *collection )
{
    Q_UNUSED( collection )
    reconnectSource();
}

void Albums::reconnectSource()
{
    dataEngine( "amarok-current" )->disconnectSource( "albums", this );
    dataEngine( "amarok-current" )->connectSource( "albums", this );
    connect( dataEngine( "amarok-current" ), SIGNAL(sourceAdded(QString)), SLOT(connectSource(QString)) );
}

#include "Albums.moc"

