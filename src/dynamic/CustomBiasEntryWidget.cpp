/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "CustomBiasEntryWidget.h"

#include "Debug.h"
#include "CustomBias.h"
#include "SliderWidget.h"

#include <KComboBox>

#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QComboBox>


Dynamic::CustomBiasEntryWidget::CustomBiasEntryWidget(Dynamic::CustomBias* bias, QWidget* parent)
    : PlaylistBrowserNS::BiasWidget( bias, parent )
    , m_cbias( bias )
{
    DEBUG_BLOCK
    // create widget with combobox, on selection, update with
    // delegate widget.
    QFrame* frame = new QFrame( parent );
    layout()->addWidget( frame );
    m_layout = new QGridLayout( frame );
    frame->setLayout( m_layout );

    QHBoxLayout* sliderLayout = new QHBoxLayout();
    m_layout->addLayout( sliderLayout, 0, 1 );

    m_weightLabel = new QLabel( " 0%", frame );
    m_weightSelection = new Amarok::Slider( Qt::Horizontal, 100, frame );
    m_weightSelection->setToolTip(
            i18n( "This controls what portion of the playlist should match the criteria" ) );
    connect( m_weightSelection, SIGNAL( valueChanged( int ) ),
            this, SLOT( weightChanged( int ) ) );
    connect( m_weightSelection, SIGNAL( valueChanged( int ) ),
             this, SIGNAL( weightChangedInt(int) ) );

    m_fieldSelection = new KComboBox( frame );

    m_layout->addWidget( new QLabel( i18n( "Proportion:" ), frame ), 0, 0 );
    m_layout->addWidget( new QLabel( i18n( "Match Type:" ), frame ), 1, 0 );

    m_layout->addWidget( m_weightSelection, 0, 1 );
    m_layout->addWidget( m_weightLabel, 0, 1 );
    m_layout->addWidget( m_fieldSelection, 1, 1 );

    sliderLayout->addWidget( m_weightSelection );
    sliderLayout->addWidget( m_weightLabel );

    int curEntry;
    for( int i = 0; i <  m_cbias->currentFactories().size(); i++)
    {
        Dynamic::CustomBiasFactory* entry = m_cbias->currentFactories().at( i );
        QVariant data;
        data.setValue( entry );
        m_fieldSelection->addItem( entry->name(), data );
        if( entry->pluginName() == m_cbias->currentEntry()->pluginName() )
            curEntry = i;
       
    }
    
    connect( m_cbias, SIGNAL( biasFactoriesChanged() ), this, SLOT( reloadBiases() ) );

    connect( m_fieldSelection, SIGNAL( activated( int ) ),
            this, SLOT( selectionChanged( int ) ) );
    m_fieldSelection->setCurrentIndex( curEntry );
    m_weightSelection->setValue( m_cbias->weight() * 100 );
    weightChanged( m_cbias->weight() * 100 );
    selectionChanged( 0 );

    //debug() << "CustomBiasEntryWidget created with weight:" << m_cbias->weight() * 100 ;

}


void
Dynamic::CustomBiasEntryWidget::selectionChanged( int index ) // SLOT
{
    DEBUG_BLOCK
    if( !m_fieldSelection )
        return;

    debug() << "selection changed to index: " << index;
    Dynamic::CustomBiasFactory* chosenFactory = m_fieldSelection->itemData( index ).value<  Dynamic::CustomBiasFactory* >();

    if( !chosenFactory )
    {
        debug() << "found a non-CustomBiasFactory in the drop-down..something bad just happened";
        return;
    }

    Dynamic::CustomBiasEntry* chosen = chosenFactory->newCustomBias( m_cbias->weight() );

    QWidget* config  = chosen->configWidget( this );
    if( !config )
    {
        debug() << "got an invalid config widget from bias type!";
        return;
    }

    // remove last item (old config widget) and old bias and add new one
    if( m_layout->rowCount() == 3 )
    {
        // remove old widget
        m_layout->removeWidget( m_currentConfig );
        delete m_currentConfig;
    }

    config->setParent( this );

    m_currentConfig = config;
    m_layout->addWidget( config, 2, 0, 1, 3, 0 );
    m_cbias->setCurrentEntry( chosen );
}

void
Dynamic::CustomBiasEntryWidget::weightChanged( int amount )
{
    double fval = (double)amount;
    m_weightLabel->setText( QString().sprintf( "%2.0f%%", fval ) );

    m_cbias->setWeight( fval / 100 );

    emit biasChanged( m_bias );
}

void Dynamic::CustomBiasEntryWidget::refreshBiasFactories()
{
    DEBUG_BLOCK

    // add any new ones
    foreach( Dynamic::CustomBiasFactory* entry, Dynamic::CustomBias::currentFactories() )
    {
        QVariant data;
        data.setValue( entry );
        if( !m_fieldSelection->contains( entry->name() ) )
        {
            debug() << "found new bias factory that wasn't in the list, so appending";
            debug() << "size of list before appending: " << m_fieldSelection->count() << "current index:" << m_fieldSelection->currentIndex();
            m_fieldSelection->addItem( entry->name(), data );
            if( m_cbias->currentEntry() && m_cbias->currentEntry()->pluginName() == entry->pluginName() )
            {
                m_fieldSelection->setCurrentItem( entry->name() );
                selectionChanged( m_fieldSelection->currentIndex() );
            }
        }
    }
    // remove and stale ones
    for( int i = 0; i < m_fieldSelection->count(); i++ )
    {
        if( !Dynamic::CustomBias::currentFactories().contains(
                m_fieldSelection->itemData( i ).value<  Dynamic::CustomBiasFactory* >() ) )
        {
            // ok, we lost one. not sure why. try to clean up sanely.
            debug() << "a bias factory was removed, updating list to reflect!";
            m_fieldSelection->removeItem( i );
        }
    }
}

