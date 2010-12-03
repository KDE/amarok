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
#include "context/widgets/TextScrollingWidget.h"
#include "TrackItem.h"

#include <Plasma/IconWidget>
#include <KConfigDialog>

#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QGraphicsLinearLayout>

Albums::Albums( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_recentCount( Amarok::config("Albums Applet").readEntry("RecentlyAdded", 5) )
    , m_rightAlignLength( Amarok::config("Albums Applet").readEntry("RightAlignLength", false) )
    , m_albumsView( 0 )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

Albums::~Albums()
{
}

void Albums::init()
{
    DEBUG_BLOCK

    // Call the base implementation.
    Context::Applet::init();

    m_headerText = new TextScrollingWidget( this );
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerText->setFont( labelFont );
    m_headerText->setText( i18n( "Recently added albums" ) );
    m_headerText->setDrawBackground( true );

    setCollapseOffHeight( -1 );
    setCollapseHeight( m_headerText->size().height()
                       + 2 * QApplication::style()->pixelMetric(QStyle::PM_LayoutTopMargin) + 6 );
    setMinimumHeight( collapseHeight() );

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
    m_albumsView->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    if( m_rightAlignLength )
        m_albumsView->setLengthAlignment( Qt::AlignRight );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical );
    layout->addItem( headerLayout );
    layout->addItem( m_albumsView );
    setLayout( layout );

    dataEngine( "amarok-current" )->connectSource( "albums", this );
    connect( CollectionManager::instance(), SIGNAL(collectionDataChanged(Collections::Collection*)),
             this, SLOT(collectionDataChanged(Collections::Collection*)) );

    updateConstraints();
}

void Albums::constraintsEvent( Plasma::Constraints constraints )
{
    Context::Applet::constraintsEvent( constraints );
    m_headerText->setScrollingText( m_headerText->text() );
    update();
}

void Albums::dataUpdated( const QString &name, const Plasma::DataEngine::Data &data )
{
    if( name != QLatin1String("albums") )
        return;

    Meta::AlbumList albums = data[ "albums" ].value<Meta::AlbumList>();
    Meta::TrackPtr track = data[ "currentTrack" ].value<Meta::TrackPtr>();
    QString headerText = data[ "headerText" ].toString();
    m_headerText->setScrollingText( headerText.isEmpty() ? i18n("Albums") : headerText );

    //Don't keep showing the albums for the artist of the last track that had album in the collection
    if( (m_currentTrack == track) && (m_albums == albums) )
    {
        debug() << "albums view data unchanged, not updating";
        return;
    }

    if( albums.isEmpty() )
    {
        setCollapseOn();
        m_albums.clear();
        return;
    }

    setCollapseOff();

    DEBUG_BLOCK
    m_albums = albums;
    m_currentTrack = track;
    m_albumsView->clear();
    m_albumsView->setMode( track ? AlbumsProxyModel::SortByYear : AlbumsProxyModel::SortByCreateDate );
    AlbumItem *currentAlbum( 0 );

    foreach( Meta::AlbumPtr albumPtr, albums )
    {
        Meta::TrackList tracks = albumPtr->tracks();
        if( tracks.isEmpty() )
            continue;

        AlbumItem *albumItem = new AlbumItem();
        albumItem->setIconSize( 50 );
        albumItem->setAlbum( albumPtr );
        albumItem->setShowArtist( !m_currentTrack );

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
            if( m_currentTrack == trackPtr )
                trackItem->bold();

            // If compilation and same artist, then highlight, but only if there's a current track
            if( m_currentTrack
                && (m_currentTrack->artist() == trackPtr->artist())
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
        if( m_currentTrack && m_currentTrack->album() == albumPtr )
            currentAlbum = albumItem;
    }

    m_albumsView->sort();
    if( currentAlbum )
    {
        m_albumsView->setRecursiveExpanded( currentAlbum, true );
        m_albumsView->scrollTo( currentAlbum );
    }

    updateConstraints();
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
    DEBUG_BLOCK
    m_albums.clear(); // clear to force an update
    Plasma::DataEngine::Data data = dataEngine( "amarok-current" )->query( "albums" );
    dataUpdated( QLatin1String("albums"), data );
}

#include "Albums.moc"
