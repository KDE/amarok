/*
    Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DynamicCategory.h"

#include "Debug.h"
#include "DynamicModel.h"
#include "amarokconfig.h"
#include "playlist/PlaylistModel.h"

#include <QLabel>
#include <QTreeWidget>

#include <KHBox>
#include <KVBox>
#include <KIcon>


namespace PlaylistBrowserNS {


DynamicCategory::DynamicCategory( QWidget* parent )
    : Amarok::Widget( parent )
{
    bool enabled = AmarokConfig::dynamicMode();

    setContentsMargins(0,0,0,0);

    QVBoxLayout* vLayout = new QVBoxLayout( this );

    //vLayout->setSizePolicy( 
            //QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored ) );

    m_onoffButton = new QPushButton( this );
    m_onoffButton->setIcon( KIcon( "amarok_dynamic" ) );
    m_onoffButton->setSizePolicy( 
            QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred ) );
    QObject::connect( m_onoffButton, SIGNAL(clicked(bool)), this, SLOT(OnOff(bool)) );


    m_repopulateButton = new QPushButton( this );
    m_repopulateButton->setText( i18n("Repopulate") );
    m_repopulateButton->setToolTip( i18n("Replace the upcoming tracks with fresh ones.") );
    m_repopulateButton->setIcon( KIcon( "view-refresh-amarok" ) );
    m_repopulateButton->setEnabled( enabled );
    m_repopulateButton->setSizePolicy( 
            QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred ) );
    QObject::connect( m_repopulateButton, SIGNAL(clicked(bool)),
            The::playlistModel(), SIGNAL(repopulate()) );
            
    initOnOffButton();


    KHBox* comboLayout = new KHBox( this );
    comboLayout->setSpacing( 10 );

    QLabel* presetLabel = new QLabel( "Preset:", comboLayout );

    m_presetComboBox = new KComboBox( comboLayout );
    m_presetComboBox->setModel( DynamicModel::instance() );
    presetLabel->setBuddy( m_presetComboBox );
    comboLayout->setStretchFactor( m_presetComboBox, 1 );

    connect( m_presetComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(playlistSelectionChanged(int) ) );

    int index = DynamicModel::instance()->retrievePlaylistIndex( 
            AmarokConfig::lastDynamicMode() );
    if( index >= 0 )
        m_presetComboBox->setCurrentIndex( index );



    KHBox *hSaveDeleteLayout = new KHBox( this );
    
    m_saveButton   = new QPushButton( hSaveDeleteLayout );
    m_saveButton->setText( i18n("Save") );
    m_saveButton->setEnabled( false );
    m_saveButton->setIcon( KIcon( "document-save-amarok" ) );
    m_saveButton->setToolTip( i18n( "Save the preset." ) );

    m_deleteButton = new QPushButton( hSaveDeleteLayout );
    m_deleteButton->setText( i18n("Delete") );
    m_deleteButton->setEnabled( false );
    m_deleteButton->setIcon( KIcon( "edit-delete-amarok" ) );
    m_deleteButton->setToolTip( i18n( "Delete the preset.") );

    



    m_biasListView = new QListView( this );
    m_biasListView->setEnabled( false );
    m_biasListView->setFrameShape( QFrame::NoFrame );

    // transparentcy
    QPalette p = m_biasListView->palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );
    m_biasListView->setPalette( p );

    vLayout->addWidget( m_onoffButton );
    vLayout->addWidget( m_repopulateButton );
    vLayout->addWidget( comboLayout );
    vLayout->addWidget( hSaveDeleteLayout );
    vLayout->addWidget( m_biasListView );

    this->setLayout( vLayout );
}


DynamicCategory::~DynamicCategory() 
{

}


void
DynamicCategory::initOnOffButton()
{
    if( AmarokConfig::dynamicMode() )
    {
        m_onoffButton->setText( i18n( "Off" ) );
        m_onoffButton->setToolTip( i18n( "Turn dynamic mode off." ) );
        m_repopulateButton->setEnabled( true );
    }
    else
    {
        m_onoffButton->setText( i18n( "On" ) );
        m_onoffButton->setToolTip( i18n( "Turn dynamic mode on." ) );
        m_repopulateButton->setEnabled( false );
    }
}


void
DynamicCategory::OnOff(bool)
{
    if( AmarokConfig::dynamicMode() ) Off();
    else                              On();
}


void
DynamicCategory::On()
{
    m_onoffButton->setText( i18n("Off") );
    AmarokConfig::setDynamicMode( true );
    // TODO: turn off other incompatible modes
    AmarokConfig::self()->writeConfig();

    initOnOffButton();

    The::playlistModel()->playlistModeChanged();  

}

void
DynamicCategory::Off()
{
    AmarokConfig::setDynamicMode( false );
    // TODO: should we restore the state of other modes?
    AmarokConfig::self()->writeConfig();

    initOnOffButton();

    The::playlistModel()->playlistModeChanged();  
}

void
DynamicCategory::playlistSelectionChanged( int index )
{
    QVariant playlist =
            DynamicModel::instance()->index( index, 0 ).data( Qt::UserRole );
    QString title = playlist.value< Dynamic::DynamicPlaylistPtr >()->title();
    AmarokConfig::setLastDynamicMode( title );
    AmarokConfig::self()->writeConfig();

    debug() << "Changing biased playlist to: " << title;
}

}


#include "DynamicCategory.moc"
