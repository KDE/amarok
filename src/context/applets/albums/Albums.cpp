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

#define DEBUG_PREFIX "Albums"

#include "Albums.h"

#include "AlbumItem.h"
#include "AlbumsView.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "EngineController.h"
#include "context/widgets/TextScrollingWidget.h"
#include "core/meta/Meta.h"
#include "TrackItem.h"

#include <Plasma/IconWidget>
#include <Plasma/Theme>
#include <KConfigDialog>

#include <QAction>
#include <QCheckBox>
#include <QFormLayout>
#include <QPainter>
#include <QSpinBox>
#include <QTreeView>
#include <QGraphicsLinearLayout>

Albums::Albums( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_recentCount( Amarok::config("Albums Applet").readEntry("RecentlyAdded", 5) )
    , m_rightAlignLength( Amarok::config("Albums Applet").readEntry("RightAlignLength", false) )
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
    m_headerText->setDrawBackground( true );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setEnabled( true );
    Plasma::IconWidget *settingsIcon = addAction( settingsAction );
    settingsIcon->setToolTip( i18n( "Settings" ) );
    connect( settingsIcon, SIGNAL(clicked()), this, SLOT(showConfigurationInterface()) );

    QGraphicsLinearLayout *headerLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    headerLayout->setContentsMargins( 0, 4, 0, 2 );
    headerLayout->addItem( m_headerText );
    headerLayout->addItem( settingsIcon );

    m_albumsView = new AlbumsView( this );
    m_albumsView->setMinimumSize( 100, 150 );
    if( m_rightAlignLength )
        m_albumsView->setLengthAlignment( Qt::AlignRight );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical );
    layout->addItem( headerLayout );
    layout->addItem( m_albumsView );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    setLayout( layout );

    dataEngine( "amarok-current" )->connectSource( "albums", this );
    connect( CollectionManager::instance(), SIGNAL(collectionDataChanged(Collections::Collection*)),
             this, SLOT(collectionDataChanged(Collections::Collection*)) );

    updateConstraints();
    update();
}

void Albums::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )
    m_headerText->setScrollingText( m_headerText->text() );
}

void Albums::dataUpdated( const QString &name, const Plasma::DataEngine::Data &data )
{
    if( name != QLatin1String("albums") )
        return;

    Meta::AlbumList albums = data[ "albums" ].value<Meta::AlbumList>();
    m_headerText->setScrollingText( data[ "headerText" ].toString() );

    //Don't keep showing the albums for the artist of the last track that had album in the collection
    if( albums.isEmpty() )
        return;

    m_albumsView->clear();
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    m_albumsView->setMode( currentTrack ? AlbumsProxyModel::SortByYear : AlbumsProxyModel::SortByCreateDate );
    const bool showArtist = !currentTrack;
    AlbumItem *currentAlbum( 0 );

    foreach( Meta::AlbumPtr albumPtr, albums )
    {
        Meta::TrackList tracks = albumPtr->tracks();
        if( tracks.isEmpty() )
            continue;

        AlbumItem *albumItem = new AlbumItem();
        albumItem->setIconSize( 50 );
        albumItem->setAlbum( albumPtr );
        albumItem->setShowArtist( showArtist );

        int numberOfDiscs = 0;
        int childRow = 0;

        qStableSort( tracks.begin(), tracks.end(), Meta::Track::lessThan );

        QMultiHash< int, TrackItem* > trackItems; // hash of tracks items for each disc
        foreach( Meta::TrackPtr trackPtr, tracks )
        {
            if( numberOfDiscs < trackPtr->discNumber() )
                numberOfDiscs = trackPtr->discNumber();

            TrackItem *trackItem = new TrackItem();
            trackItem->setTrack( trackPtr );

            // bold the current track to make it more visible
            if( currentTrack == trackPtr )
                trackItem->bold();

            // If compilation and same artist, then highlight, but only if there's a current track
            if( currentTrack
                && (currentTrack->artist() == trackPtr->artist())
                && albumPtr->isCompilation() )
            {
                trackItem->italicise();
            }
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

        m_albumsView->appendAlbum( albumItem );
        if( currentTrack && currentTrack->album() == albumPtr )
            currentAlbum = albumItem;
    }

    m_albumsView->sort();
    if( currentAlbum )
    {
        m_albumsView->setRecursiveExpanded( currentAlbum, true );
        m_albumsView->scrollTo( currentAlbum );
    }

    updateConstraints();
    update();
}

void Albums::createConfigurationInterface( KConfigDialog *parent )
{
    QSpinBox *spinBox = new QSpinBox;
    spinBox->setRange( 1, 100 );
    spinBox->setValue( m_recentCount );
    connect( spinBox, SIGNAL(valueChanged(int)), SLOT(setRecentCount(int)) );

    QCheckBox *checkBox = new QCheckBox( i18n( "Enabled" ) );
    checkBox->setCheckState( m_rightAlignLength ? Qt::Checked : Qt::Unchecked );
    connect( checkBox, SIGNAL(stateChanged(int)), SLOT(setRightAlignLength(int)) );

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow( i18n("Number of recently added albums:"), spinBox );
    formLayout->addRow( i18n("Right align track lengths:"), checkBox );

    QWidget *config = new QWidget;
    config->setLayout( formLayout );

    parent->addPage( config, i18n( "Albums Applet Settings" ), "preferences-system");
    connect( parent, SIGNAL(accepted()), this, SLOT(saveConfiguration()) );
}

void Albums::setRecentCount( int val )
{
    m_recentCount = val;
}

void Albums::setRightAlignLength( int state )
{
    m_rightAlignLength = (state == Qt::Checked );
    m_albumsView->setLengthAlignment( m_rightAlignLength ? Qt::AlignRight : Qt::AlignLeft );
}

void Albums::saveConfiguration()
{
    Amarok::config("Albums Applet").writeEntry( "RecentlyAdded", QString::number( m_recentCount ) );
    Amarok::config("Albums Applet").writeEntry( "RightAlignLength", m_rightAlignLength );
    Plasma::DataEngine::Data data = dataEngine( "amarok-current" )->query( "albums" );
    dataUpdated( QLatin1String("albums"), data );
}

void Albums::collectionDataChanged( Collections::Collection *collection )
{
    Q_UNUSED( collection )
    Plasma::DataEngine::Data data = dataEngine( "amarok-current" )->query( "albums" );
    dataUpdated( QLatin1String("albums"), data );
}

#include "Albums.moc"
