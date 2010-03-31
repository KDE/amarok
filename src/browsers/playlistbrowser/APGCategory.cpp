/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#define DEBUG_PREFIX "APGCategory"

#include "APGCategory.h"

#include "amarokconfig.h"
#include "playlistgenerator/ConstraintSolver.h"
#include "playlistgenerator/PresetModel.h"
#ifndef KDE_NO_DEBUG_OUTPUT
#include "playlistgenerator/ConstraintTestWrapper.h"
#endif

#include "core/support/Debug.h"

#include <KIcon>

#include <QModelIndex>
#include <QtGui>

PlaylistBrowserNS::APGCategory::APGCategory( QWidget* )
    : BrowserCategory ( "APG", 0 )
{
    m_qualityFactor = AmarokConfig::qualityFactorAPG();

    setPrettyName( i18n( "Automated Playlist Generator" ) );
    setShortDescription( i18n("Create playlists by specifying criteria") );
    setIcon( KIcon( "playlist-generator" ) );

    setLongDescription( i18n("Create playlists by specifying criteria") );

    setContentsMargins( 0, 0, 0, 0 );

    APG::PresetModel* presetmodel = APG::PresetModel::instance();
    connect( presetmodel, SIGNAL( lock( bool ) ), this, SLOT( setDisabled( bool ) ) );

    /* Create the toolbar -- Qt's Designer doesn't let us put a toolbar
     * anywhere except in a MainWindow, so we've got to create it by hand here. */
    QToolBar* toolBar_Actions = new QToolBar( this );
    toolBar_Actions->setMovable( false );
    toolBar_Actions->setFloatable( false );
    toolBar_Actions->setIconSize( QSize( 22, 22 ) );
    toolBar_Actions->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );

    QAction* a;
    a = toolBar_Actions->addAction( KIcon( "list-add-amarok" ), i18n("Add new preset") );
    connect( a, SIGNAL( triggered( bool ) ), presetmodel, SLOT( addNew() ) );

    a = toolBar_Actions->addAction( KIcon( "document-properties-amarok" ), i18n("Edit selected preset") );
    a->setEnabled( false );
    connect( a, SIGNAL( triggered( bool ) ), presetmodel, SLOT( edit() ) );
    connect( this, SIGNAL( validIndexSelected( bool ) ), a, SLOT( setEnabled( bool ) ) );

    a = toolBar_Actions->addAction( KIcon( "list-remove-amarok" ), i18n("Delete selected preset") );
    a->setEnabled( false );
    connect( a, SIGNAL( triggered( bool ) ), presetmodel, SLOT( removeActive() ) );
    connect( this, SIGNAL( validIndexSelected( bool ) ), a, SLOT( setEnabled( bool ) ) );

    a = toolBar_Actions->addAction( KIcon( "document-import-amarok" ), i18n("Import a new preset") );
    a->setEnabled( true );
    connect( a, SIGNAL( triggered( bool ) ), presetmodel, SLOT( import() ) );

    a = toolBar_Actions->addAction( KIcon( "document-export-amarok" ), i18n("Export the selected preset") );
    a->setEnabled( false );
    connect( a, SIGNAL( triggered( bool ) ), presetmodel, SLOT( exportActive() ) );
    connect( this, SIGNAL( validIndexSelected( bool ) ), a, SLOT( setEnabled( bool ) ) );

#ifndef KDE_NO_DEBUG_OUTPUT
    toolBar_Actions->addSeparator();

    a = toolBar_Actions->addAction( KIcon( "flag-amarok" ), "Run constraint tester" );
    a->setEnabled( false );
    connect( this, SIGNAL( validIndexSelected( bool ) ), a, SLOT( setEnabled( bool ) ) );
    APG::ConstraintTestWrapper* ctw = new APG::ConstraintTestWrapper( this );
    connect( a, SIGNAL( triggered( bool ) ), ctw, SLOT( runTest() ) );
#endif

    toolBar_Actions->addSeparator();

    a = toolBar_Actions->addAction( KIcon( "go-next-amarok" ), i18n("Run APG with selected preset") );
    a->setEnabled( false );
    connect( a, SIGNAL( triggered( bool ) ), this, SLOT( runGenerator() ) );
    connect( this, SIGNAL( validIndexSelected( bool ) ), a, SLOT( setEnabled( bool ) ) );

    /* Create the preset list view */
    QLabel* label_Title = new QLabel( i18n("APG Presets"), this );
    label_Title->setAlignment( Qt::AlignCenter );

    // transparent background for list view
    // TODO: make the rows a little more distinguished, like static playlist browser
    QPalette p = palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );
    p.setColor( QPalette::Window, c );

    QListView* listView = new QListView( this );
    listView->setModel( presetmodel );
    listView->setSelectionMode( QAbstractItemView::SingleSelection );
    listView->setPalette( p );
    listView->setFrameShape( QFrame::NoFrame );
    listView->setAutoFillBackground( false );
    connect( listView->selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( activeChanged( const QModelIndex& ) ) );
    connect( listView, SIGNAL( doubleClicked( const QModelIndex& ) ), presetmodel, SLOT( editPreset( const QModelIndex& ) ) );

    // Speed/Quality tradeoff slider
    QLabel* label_Tradeoff = new QLabel( i18n("Generator Optimization"), this );
    label_Tradeoff->setAlignment( Qt::AlignCenter );

    QFrame* qual_Frame = new QFrame( this );
    QLabel* label_Speed = new QLabel( i18n("Speed"), qual_Frame );
    QSlider* qual_Slider = new QSlider( Qt::Horizontal, qual_Frame );
    qual_Slider->setRange( 0, APG::ConstraintSolver::QUALITY_RANGE );
    qual_Slider->setValue( m_qualityFactor );
    connect( qual_Slider, SIGNAL( sliderMoved( int ) ), this, SLOT ( setQualityFactor( int ) ) );
    QLabel* label_Quality = new QLabel( i18n("Accuracy"), qual_Frame );

    QLayout* qf_Layout = new QHBoxLayout( qual_Frame );
    qf_Layout->addWidget( label_Speed );
    qf_Layout->addWidget( qual_Slider );
    qf_Layout->addWidget( label_Quality );
    qual_Frame->setLayout( qf_Layout );

    QMetaObject::connectSlotsByName( this );
}

PlaylistBrowserNS::APGCategory::~APGCategory()
{
    APG::PresetModel::destroy();
    AmarokConfig::setQualityFactorAPG( m_qualityFactor );
    AmarokConfig::self()->writeConfig();
}

void
PlaylistBrowserNS::APGCategory::activeChanged( const QModelIndex& index )
{
    APG::PresetModel::instance()->setActivePreset( index );
    emit validIndexSelected( index.isValid() );
}

void
PlaylistBrowserNS::APGCategory::setQualityFactor( int f )
{
    m_qualityFactor = f;
}

void
PlaylistBrowserNS::APGCategory::runGenerator()
{
    APG::PresetModel::instance()->runGenerator( m_qualityFactor );
}
