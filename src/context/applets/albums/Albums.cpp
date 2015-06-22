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
#include "core/collections/Collection.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "context/widgets/AppletHeader.h"
#include "TrackItem.h"

#include <KConfigDialog>
#include <KLineEdit>
#include <Plasma/IconWidget>

#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

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
    DEBUG_BLOCK

    // Call the base implementation.
    Context::Applet::init();

    enableHeader( true );
    setHeaderText( i18n( "Recently Added Albums" ) );

    setCollapseOffHeight( -1 );
    setCollapseHeight( m_header->height() );
    setMinimumHeight( collapseHeight() );

    QAction *settingsAction = new QAction( this );
    settingsAction->setIcon( QIcon::fromTheme( "preferences-system" ) );
    settingsAction->setEnabled( true );
    settingsAction->setToolTip( i18n( "Settings" ) );
    addRightHeaderAction( settingsAction );
    connect( settingsAction, SIGNAL(triggered()), this, SLOT(showConfigurationInterface()) );

    QAction *filterAction = new QAction( this );
    filterAction->setIcon( QIcon::fromTheme( "view-filter" ) );
    filterAction->setEnabled( true );
    filterAction->setToolTip( i18n( "Filter Albums" ) );
    m_filterIcon = addLeftHeaderAction( filterAction );
    connect( filterAction, SIGNAL(triggered()), this, SLOT(showFilterBar()) );

    m_albumsView = new AlbumsView( this );
    m_albumsView->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    if( m_rightAlignLength )
        m_albumsView->setLengthAlignment( Qt::AlignRight );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical );
    layout->addItem( m_header );
    layout->addItem( m_albumsView );
    setLayout( layout );

    dataEngine( "amarok-current" )->connectSource( "albums", this );
    connect( CollectionManager::instance(), SIGNAL(collectionDataChanged(Collections::Collection*)),
             this, SLOT(collectionDataChanged(Collections::Collection*)) );

    updateConstraints();
}

void Albums::showFilterBar()
{
    m_filterIcon->setEnabled( false );
    AlbumsFilterBar *bar = new AlbumsFilterBar( this );
    bar->setContentsMargins( 0, 0, 0, 0 );
    QGraphicsLinearLayout *l = static_cast<QGraphicsLinearLayout*>( layout() );
    l->setItemSpacing( 1, 0 );
    l->addItem( bar );
    connect( bar, SIGNAL(filterTextChanged(QString)), this, SLOT(filterTextChanged(QString)) );
    connect( bar, SIGNAL(closeRequested()), this, SLOT(closeFilterBar()) );
    bar->focusEditor();
}

void Albums::closeFilterBar()
{
    filterTextChanged( QString() );
    AlbumsFilterBar *bar = static_cast<AlbumsFilterBar*>( sender() );
    QGraphicsLinearLayout *l = static_cast<QGraphicsLinearLayout*>( layout() );
    l->removeItem( bar );
    bar->deleteLater();
    m_filterIcon->setEnabled( true );
}

void Albums::filterTextChanged( const QString &text )
{
    m_albumsView->setFilterPattern( text );
}

void Albums::dataUpdated( const QString &name, const Plasma::DataEngine::Data &data )
{
    if( name != QLatin1String("albums") )
        return;

    Meta::AlbumList albums = data[ "albums" ].value<Meta::AlbumList>();
    Meta::TrackPtr track = data[ "currentTrack" ].value<Meta::TrackPtr>();
    QString headerText = data[ "headerText" ].toString();
    setHeaderText( headerText.isEmpty() ? i18n("Albums") : headerText );

    //Don't keep showing the albums for the artist of the last track that had album in the collection
    if( (m_currentTrack == track) && (m_albums == albums) )
        return;

    if( albums.isEmpty() )
    {
        debug() << "received albums is empty";
        setCollapseOn();
        m_albums.clear();
        m_albumsView->clear();
        return;
    }

    setCollapseOff();

    m_albums = albums;
    m_currentTrack = track;
    m_albumsView->clear();
    m_albumsView->setMode( track ? AlbumsProxyModel::SortByYear : AlbumsProxyModel::SortByCreateDate );
    QStandardItem *currentItem( 0 );

    foreach( Meta::AlbumPtr albumPtr, albums )
    {
        // do not show all tracks without an album from the collection, this takes ages
        // TODO: show all tracks from this artist that are not part of an album
        if( albumPtr->name().isEmpty() )
            continue;

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
            if( m_currentTrack && *m_currentTrack == *trackPtr )
            {
                currentItem = trackItem;
                trackItem->bold();
            }

            // If compilation and same artist, then highlight, but only if there's a current track
            if( m_currentTrack
                && m_currentTrack->artist() && trackPtr->artist()
                && (*m_currentTrack->artist() == *trackPtr->artist())
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
                if( numberOfDiscs > 1 )
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
    }

    m_albumsView->sort();
    if( currentItem )
    {
        m_albumsView->setRecursiveExpanded( currentItem, true );
        m_albumsView->scrollTo( currentItem );
    }

    updateConstraints();
}

void Albums::createConfigurationInterface( KConfigDialog *parent )
{
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    parent->setLayout(mainLayout);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    parent->connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    parent->connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(buttonBox);

    QSpinBox *spinBox = new QSpinBox;
    spinBox->setRange( 1, 100 );
    spinBox->setValue( m_recentCount );
    connect( spinBox, SIGNAL(valueChanged(int)), SLOT(setRecentCount(int)) );

    QCheckBox *checkBox = new QCheckBox( i18n( "Right align track lengths" ) );
    checkBox->setCheckState( m_rightAlignLength ? Qt::Checked : Qt::Unchecked );
    connect( checkBox, SIGNAL(stateChanged(int)), SLOT(setRightAlignLength(int)) );

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow( i18n("Number of recently added albums:"), spinBox );
    formLayout->addRow( checkBox );

    QWidget *config = new QWidget;
    config->setLayout( formLayout );

    parent->addPage( config, i18n( "Albums Applet Settings" ), "preferences-system");
    connect( parent, SIGNAL(accepted()), this, SLOT(saveConfiguration()) );
}

void Albums::keyPressEvent( QKeyEvent *event )
{
    if( event->key() == Qt::Key_Slash || event->matches( QKeySequence::Find ) )
    {
        if( m_filterIcon->isEnabled() )
        {
            showFilterBar();
            event->accept();
            return;
        }
    }
    Context::Applet::keyPressEvent( event );
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

    // clear to force an update
    m_albums.clear();

    Plasma::DataEngine::Data data = dataEngine( "amarok-current" )->query( "albums" );
    dataUpdated( QLatin1String("albums"), data );
}

void Albums::collectionDataChanged( Collections::Collection *collection )
{
    Q_UNUSED( collection )

    DEBUG_BLOCK
}

AlbumsFilterBar::AlbumsFilterBar( QGraphicsItem *parent, Qt::WindowFlags wFlags )
    : QGraphicsWidget( parent, wFlags )
    , m_editor( new KLineEdit )
    , m_closeIcon( new Plasma::IconWidget( QIcon::fromTheme("dialog-close"), QString(), this ) )
{
    QGraphicsProxyWidget *editProxy = new QGraphicsProxyWidget( this );
    editProxy->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    editProxy->setWidget( m_editor );

    m_editor->installEventFilter( this );
    m_editor->setAttribute( Qt::WA_NoSystemBackground );
    m_editor->setAutoFillBackground( true );
    m_editor->setClearButtonShown( true );
    m_editor->setClickMessage( i18n( "Filter Albums" ) );
    m_editor->setContentsMargins( 0, 0, 0, 0 );

    QSizeF iconSize = m_closeIcon->sizeFromIconSize( 16 );
    m_closeIcon->setMaximumSize( iconSize );
    m_closeIcon->setMinimumSize( iconSize );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Horizontal, this );
    layout->setSpacing( 1 );
    layout->addItem( editProxy );
    layout->addItem( m_closeIcon );
    layout->setStretchFactor( editProxy, 100 );
    layout->setAlignment( editProxy, Qt::AlignCenter );
    layout->setAlignment( m_closeIcon, Qt::AlignCenter );
    layout->setContentsMargins( 0, 2, 0, 0 );

    m_closeIcon->setToolTip( i18n( "Close" ) );
    connect( m_closeIcon, SIGNAL(clicked()), SIGNAL(closeRequested()) );
    connect( m_editor, SIGNAL(textChanged(QString)), SIGNAL(filterTextChanged(QString)) );
}

bool
AlbumsFilterBar::eventFilter( QObject *obj, QEvent *e )
{
    if( obj == m_editor )
    {
        if( e->type() == QEvent::KeyPress )
        {
            QKeyEvent *kev = static_cast<QKeyEvent*>( e );
            if( kev->key() == Qt::Key_Escape )
            {
                kev->accept();
                emit closeRequested();
                return true;
            }
        }
    }
    return QGraphicsWidget::eventFilter( obj, e );
}

void
AlbumsFilterBar::focusEditor()
{
    m_editor->setFocus( Qt::PopupFocusReason );
}

