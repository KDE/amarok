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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>


namespace PlaylistBrowserNS {


DynamicCategory::DynamicCategory( QWidget* parent )
    : Amarok::Widget( parent )
{
    bool enabled = AmarokConfig::dynamicMode();

    setContentsMargins(0,0,0,0);

    QVBoxLayout* vLayout = new QVBoxLayout( this );

    QHBoxLayout *hSaveLoadLayout = new QHBoxLayout( this );
    
    m_saveButton   = new QPushButton( this );
    m_saveButton->setText( i18n("Save") );
    m_saveButton->setEnabled( false );
    m_saveButton->setSizePolicy(
            QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred ) );
    hSaveLoadLayout->addWidget( m_saveButton );

    m_deleteButton = new QPushButton( this );
    m_deleteButton->setText( i18n("Delete") );
    m_deleteButton->setEnabled( false );
    m_deleteButton->setSizePolicy(
            QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred ) );
    hSaveLoadLayout->addWidget( m_deleteButton );
    

    m_onoffButton = new QPushButton( this );
    m_onoffButton->setText( enabled ? i18n("Off") : i18n("On") );
    m_onoffButton->setSizePolicy( 
            QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred ) );
    QObject::connect( m_onoffButton, SIGNAL(clicked(bool)), this, SLOT(OnOff(bool)) );

    m_repopulateButton = new QPushButton( this );
    m_repopulateButton->setText( i18n("Repopulate") );
    m_repopulateButton->setEnabled( enabled );
    m_repopulateButton->setSizePolicy( 
            QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred ) );
    QObject::connect( m_repopulateButton, SIGNAL(clicked(bool)),
            The::playlistModel(), SIGNAL(repopulate()) );
            

    m_presetComboBox = new QComboBox( this );
    m_presetComboBox->setModel( DynamicModel::instance() );
    connect( m_presetComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(playlistSelectionChanged(int) ) );

    int index = DynamicModel::instance()->retrievePlaylistIndex( 
            AmarokConfig::lastDynamicMode() );
    if( index >= 0 )
        m_presetComboBox->setCurrentIndex( index );

    m_dynamicTreeView = new QTreeWidget( this );
    m_dynamicTreeView->setEnabled( false );
    m_dynamicTreeView->setFrameShape( QFrame::NoFrame );



    QStringList warning;
    warning += "Abandon all hope!\nThis is a work in progress.\nExpect nothing!";
    ((QTreeWidget*)m_dynamicTreeView)->setCurrentItem(
        new QTreeWidgetItem( (QTreeWidget*)m_dynamicTreeView, warning ) );


    vLayout->addWidget( m_onoffButton );
    vLayout->addWidget( m_repopulateButton );
    vLayout->addWidget( m_presetComboBox );
    vLayout->addWidget( m_saveButton );
    vLayout->addWidget( m_deleteButton );
    vLayout->addWidget( m_dynamicTreeView );
}


DynamicCategory::~DynamicCategory() 
{

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

    The::playlistModel()->playlistModeChanged();  

    m_repopulateButton->setEnabled( true );
}

void
DynamicCategory::Off()
{
    m_onoffButton->setText( i18n("On") );
    AmarokConfig::setDynamicMode( false );
    // TODO: should we restore the state of other modes?
    AmarokConfig::self()->writeConfig();

    The::playlistModel()->playlistModeChanged();  

    m_repopulateButton->setEnabled( false );
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
