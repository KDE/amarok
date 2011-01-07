/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009-2010 Leo Franchi <lfranchi@kde.org>                               *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "DynamicCategory.h"

#include "DynamicModel.h"
#include "DynamicBiasWidgets.h"
#include "amarokconfig.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"

// #include <QStandardItemModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>

#include <QScrollArea>
#include <QCheckBox>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QSpinBox>

#include <KHBox>
#include <KIcon>
#include <KStandardDirs>
#include <KSeparator>
#include <KToolBar>
#include <KComboBox>


namespace PlaylistBrowserNS {


DynamicCategory::DynamicCategory( QWidget* parent )
    : BrowserCategory( "dynamic category", parent )
{
    setPrettyName( i18n( "Dynamic Playlists" ) );
    setShortDescription( i18n( "Dynamically updating parameter based playlists" ) );
    setIcon( KIcon( "dynamic-amarok" ) );

    setLongDescription( i18n( "With a dynamic playlist, Amarok becomes your own personal dj, automatically selecting tracks for you, based on a number of parameters that you select." ) );

    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_dynamic_playlists.png" ) );

    bool enabled = AmarokConfig::dynamicMode();

    setContentsMargins(0,0,0,0);

    KHBox* controls1Layout = new KHBox( this );

    m_onOffCheckbox = new QCheckBox( controls1Layout );
    m_onOffCheckbox->setIcon( KIcon( "dynamic-amarok" ) );
    m_onOffCheckbox->setText( i18nc( "Turn dynamic mode on", "On" ) );
    m_onOffCheckbox->setToolTip( i18n( "Turn dynamic mode on." ) );
    m_onOffCheckbox->setCheckable( true );
    m_onOffCheckbox->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
    QObject::connect( m_onOffCheckbox, SIGNAL( toggled( bool ) ),
                      this, SLOT( enableDynamicMode( bool ) ) );

    m_repopulateButton = new QPushButton( controls1Layout );
    m_repopulateButton->setText( i18n("Repopulate") );
    m_repopulateButton->setToolTip( i18n("Replace the upcoming tracks with fresh ones.") );
    m_repopulateButton->setIcon( KIcon( "view-refresh-amarok" ) );
    m_repopulateButton->setEnabled( enabled );
    m_repopulateButton->setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred ) );
    QObject::connect( m_repopulateButton, SIGNAL( clicked(bool) ), The::playlistActions(), SLOT( repopulateDynamicPlaylist() ) );


    new KSeparator( Qt::Horizontal, this );


    KHBox* controls2Layout = new KHBox( this );

    new QLabel( i18n( "Previous:" ), controls2Layout );

    m_previous = new QSpinBox( controls2Layout );
    m_previous->setMinimum( 0 );
    m_previous->setToolTip( i18n( "Number of previous tracks to remain in the playlist." ) );
    m_previous->setValue( AmarokConfig::previousTracks() );
    QObject::connect( m_previous, SIGNAL( valueChanged( int ) ), this, SLOT( setPreviousTracks( int ) ) );

    new QLabel( i18n( "Upcoming:" ), controls2Layout );

    m_upcoming = new QSpinBox( controls2Layout );
    m_upcoming->setMinimum( 1 );
    m_upcoming->setToolTip( i18n( "Number of upcoming tracks to add to the playlist." ) );
    m_upcoming->setValue( AmarokConfig::upcomingTracks() );
    QObject::connect( m_upcoming, SIGNAL( valueChanged( int ) ), this, SLOT( setUpcomingTracks( int ) ) );


    QObject::connect( (const QObject*)Amarok::actionCollection()->action( "playlist_clear" ),  SIGNAL( triggered( bool ) ),  this, SLOT( playlistCleared() ) );
    QObject::connect( (const QObject*)Amarok::actionCollection()->action( "disable_dynamic" ),  SIGNAL( triggered( bool ) ),  this, SLOT( playlistCleared() ), Qt::DirectConnection );



    KHBox* presetLayout = new KHBox( this );


    QLabel* presetLabel = new QLabel( i18n( "Playlist:" ), presetLayout );

    m_presetComboBox = new KComboBox( presetLayout );
    m_presetComboBox->setModel( DynamicModel::instance() );

    connect( DynamicModel::instance(), SIGNAL( activeChanged( int ) ),
            m_presetComboBox, SLOT(setCurrentIndex(int)) );

    connect( m_presetComboBox, SIGNAL(currentIndexChanged( int ) ),
            this, SLOT(playlistSelectionChanged( int ) ) );

    presetLabel->setBuddy( m_presetComboBox );

    presetLayout->setStretchFactor( m_presetComboBox, 1 );



    KToolBar* presetToolbar = new KToolBar( presetLayout );
    presetToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    presetToolbar->setMovable( false );
    presetToolbar->setFloatable( false );
    presetToolbar->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );
    m_saveButton   = new QToolButton( presetToolbar );
    //m_saveButton->setText( i18n("Save") );
    m_saveButton->setIcon( KIcon( "document-save-amarok" ) );
    m_saveButton->setToolTip( i18n( "Save the preset." ) );
    presetToolbar->addWidget( m_saveButton );

    connect( m_saveButton, SIGNAL( clicked( bool ) ), SLOT( save() ) );


    m_deleteButton = new QToolButton( presetToolbar );
    //m_deleteButton->setText( i18n("Delete") );
    m_deleteButton->setEnabled( false );
    m_deleteButton->setIcon( KIcon( "edit-delete-amarok" ) );
    m_deleteButton->setToolTip( i18n( "Delete the preset.") );
    presetToolbar->addWidget( m_deleteButton );

    connect( m_deleteButton, SIGNAL(clicked(bool)),
            DynamicModel::instance(), SLOT(removeActive()) );

    m_scroller = new QScrollArea( this );
    m_scroller->setWidgetResizable( true );

    int index = DynamicModel::instance()->activePlaylistIndex();

    debug() << "Setting index: " << index;
    if( index >= 0 )
    {
        m_presetComboBox->setCurrentIndex( index );
        playlistSelectionChanged( index );
    }
    else
    {
        m_presetComboBox->setCurrentIndex( 0 );
        playlistSelectionChanged( 0 );
    }

    m_onOffCheckbox->setChecked( AmarokConfig::dynamicMode() );
}


DynamicCategory::~DynamicCategory()
{
    saveOnExit();
}


void
DynamicCategory::enableDynamicMode( bool enable )
{
    if( AmarokConfig::dynamicMode() == enable )
        return;

    AmarokConfig::setDynamicMode( enable );
    // TODO: turn off other incompatible modes
    // TODO: should we restore the state of other modes?
    AmarokConfig::self()->writeConfig();
    m_repopulateButton->setEnabled( enable );

    //if the playlist is empty, repopulate while we are at it:

    if( enable )
    {
        if ( Playlist::ModelStack::instance()->bottom()->rowCount() == 0 )
            The::playlistActions()->repopulateDynamicPlaylist();
    }

    The::playlistActions()->playlistModeChanged();
}


void
DynamicCategory::playlistCleared() // SLOT
{
    // we have a whole method here b/c we  don't want to do any extra work on each clear press
    if( AmarokConfig::dynamicMode() ) // only do anything if dynamic mode was on
    {
        AmarokConfig::setDynamicMode( false );
        // TODO: should we restore the state of other modes?
        AmarokConfig::self()->writeConfig();
        m_repopulateButton->setEnabled( false );
        m_onOffCheckbox->setChecked( false );
        The::playlistActions()->playlistModeChanged();
    }
}

void
DynamicCategory::setUpcomingTracks( int n ) // SLOT
{
    if( n >= 1 )
        AmarokConfig::setUpcomingTracks( n );
}

void
DynamicCategory::setPreviousTracks( int n ) // SLOT
{
    if( n >= 0 )
        AmarokConfig::setPreviousTracks( n );
}

void
DynamicCategory::playlistSelectionChanged( int index )
{
    DEBUG_BLOCK
    Dynamic::BiasedPlaylist* playlist = qobject_cast<Dynamic::BiasedPlaylist*>(DynamicModel::instance()->setActivePlaylist( index ));

    if( !playlist )
        return;

    if( DynamicModel::instance()->isActiveDefault() )
        m_deleteButton->setEnabled( false );
    else
        m_deleteButton->setEnabled( true );


    if( !DynamicModel::instance()->isActiveUnsaved() )
    {
        AmarokConfig::setLastDynamicMode( playlist->title() );
        AmarokConfig::self()->writeConfig();
    }

    delete m_scroller->takeWidget();
    m_scroller->setWidget( new PlaylistBrowserNS::BiasBoxWidget( playlist->bias(), m_scroller ) );
    m_scroller->widget()->show();

    debug() << "Changing biased playlist to: " << playlist->title();
}

void
DynamicCategory::save()
{
    bool ok;
    QString title =
        QInputDialog::getText( this, i18n("Playlist Name"),
                               i18n("Enter a name for the playlist:"),
                               QLineEdit::Normal,
                               DynamicModel::instance()->activePlaylist()->title(),
                               &ok );
    if( !ok ) return;

    // TODO: write a custom dialog to prevent this from happening in the first
    // place
    DynamicModel* model = DynamicModel::instance();
    if( model->playlistIndex( title ) == model->defaultPlaylistIndex() )
    {
        QMessageBox::warning( this, i18n( "Warning" ),
                              i18n( "Cannot overwrite the random playlist." ) );
        return;
    }

    model->saveActive( title );
    playlistSelectionChanged( model->playlistIndex( title ) );
}

void
DynamicCategory::saveOnExit()
{
    DEBUG_BLOCK

    DynamicModel::instance()->saveCurrentPlaylists();
}

} // namespace

#include "DynamicCategory.moc"

