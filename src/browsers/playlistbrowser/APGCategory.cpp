/****************************************************************************************
 * Copyright (c) 2008-2012 Soren Harward <stharward@gmail.com>                          *
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

#include "APGCategory.h"

#include "amarokconfig.h"
#include "playlistgenerator/ConstraintSolver.h"
#include "playlistgenerator/PresetModel.h"
#include "widgets/PrettyTreeView.h"

#include <KLocalizedString>

#include <QAction>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QModelIndex>
#include <QToolBar>

PlaylistBrowserNS::APGCategory::APGCategory( QWidget* )
    : BrowserCategory ( QStringLiteral("APG"), nullptr )
{
    m_qualityFactor = AmarokConfig::qualityFactorAPG();

    setPrettyName( i18n( "Automated Playlist Generator" ) );
    setShortDescription( i18n("Create playlists by specifying criteria") );
    setIcon( QIcon::fromTheme( QStringLiteral("playlist-generator") ) );

    // set background
    if( AmarokConfig::showBrowserBackgroundImage() )
        setBackgroundImage( imagePath() );

    setLongDescription( i18n("Create playlists by specifying criteria") );

    setContentsMargins( 0, 0, 0, 0 );

    APG::PresetModel* presetmodel = APG::PresetModel::instance();
    connect( presetmodel, &APG::PresetModel::lock, this, &APGCategory::setDisabled );

    /* Create the toolbar -- Qt's Designer doesn't let us put a toolbar
     * anywhere except in a MainWindow, so we've got to create it by hand here. */
    QToolBar* toolBar_Actions = new QToolBar( this );
    toolBar_Actions->setMovable( false );
    toolBar_Actions->setFloatable( false );
    toolBar_Actions->setIconSize( QSize( 22, 22 ) );
    toolBar_Actions->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );

    QAction* a;
    a = toolBar_Actions->addAction( QIcon::fromTheme( QStringLiteral("list-add-amarok") ), i18n("Add new preset") );
    connect( a, &QAction::triggered, presetmodel, &APG::PresetModel::addNew );

    a = toolBar_Actions->addAction( QIcon::fromTheme( QStringLiteral("document-properties-amarok") ), i18n("Edit selected preset") );
    a->setEnabled( false );
    connect( a, &QAction::triggered, presetmodel, &APG::PresetModel::edit );
    connect( this, &APGCategory::validIndexSelected, a, &QAction::setEnabled );

    a = toolBar_Actions->addAction( QIcon::fromTheme( QStringLiteral("list-remove-amarok") ), i18n("Delete selected preset") );
    a->setEnabled( false );
    connect( a, &QAction::triggered, presetmodel, &APG::PresetModel::removeActive );
    connect( this, &APGCategory::validIndexSelected, a, &QAction::setEnabled );

    a = toolBar_Actions->addAction( QIcon::fromTheme( QStringLiteral("document-import-amarok") ), i18n("Import a new preset") );
    a->setEnabled( true );
    connect( a, &QAction::triggered, presetmodel, &APG::PresetModel::import );

    a = toolBar_Actions->addAction( QIcon::fromTheme( QStringLiteral("document-export-amarok") ), i18n("Export the selected preset") );
    a->setEnabled( false );
    connect( a, &QAction::triggered, presetmodel, &APG::PresetModel::exportActive );
    connect( this, &APGCategory::validIndexSelected, a, &QAction::setEnabled );

    toolBar_Actions->addSeparator();

    a = toolBar_Actions->addAction( QIcon::fromTheme( QStringLiteral("go-next-amarok") ), i18n("Run APG with selected preset") );
    a->setEnabled( false );
    connect( a, &QAction::triggered, this, &APGCategory::runGenerator );
    connect( this, &APGCategory::validIndexSelected, a, &QAction::setEnabled );

    /* Create the preset list view */
    QLabel* label_Title = new QLabel( i18n("APG Presets"), this );
    label_Title->setAlignment( Qt::AlignCenter );

    Amarok::PrettyTreeView* listView = new Amarok::PrettyTreeView( this );
    listView->setHeaderHidden( true );
    listView->setRootIsDecorated( false );
    listView->setModel( presetmodel );
    listView->setSelectionMode( QAbstractItemView::SingleSelection );
    listView->setFrameShape( QFrame::NoFrame );
    listView->setAutoFillBackground( false );
    connect( listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &APGCategory::activeChanged );
    connect( listView, &Amarok::PrettyTreeView::doubleClicked, presetmodel, &APG::PresetModel::editPreset );

    // Speed/Quality tradeoff slider
    QLabel* label_Tradeoff = new QLabel( i18n("Generator Optimization"), this );
    label_Tradeoff->setAlignment( Qt::AlignCenter );

    QFrame* qual_Frame = new QFrame( this );
    QLabel* label_Speed = new QLabel( i18n("Speed"), qual_Frame );
    QSlider* qual_Slider = new QSlider( Qt::Horizontal, qual_Frame );
    qual_Slider->setRange( 0, APG::ConstraintSolver::QUALITY_RANGE );
    qual_Slider->setValue( m_qualityFactor );
    connect( qual_Slider, &QSlider::sliderMoved, this, &APGCategory::setQualityFactor );
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
    AmarokConfig::self()->save();
}

void
PlaylistBrowserNS::APGCategory::activeChanged( const QModelIndex& index )
{
    APG::PresetModel::instance()->setActivePreset( index );
    Q_EMIT validIndexSelected( index.isValid() );
}

void
PlaylistBrowserNS::APGCategory::setQualityFactor( int f )
{
    m_qualityFactor = f;
}

void
PlaylistBrowserNS::APGCategory::runGenerator()
{
    APG::PresetModel::instance()->savePresetsToXmlDefault();
    APG::PresetModel::instance()->runGenerator( m_qualityFactor );
}
