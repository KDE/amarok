/****************************************************************************************
 * Copyright (c) 2009, 2010 Leo Franchi <lfranchi@kde.org>                              *
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

    int currentEntry = 0;
    for( int i = 0; i <  m_cbias->currentFactories().size(); i++ )
    {
        Dynamic::CustomBiasFactory* entry = m_cbias->currentFactories().at( i );
        QVariant data;
        data.setValue( entry );
        m_fieldSelection->addItem( entry->name(), data );
        if( m_cbias->currentEntry() )
            if( entry->pluginName() == m_cbias->currentEntry()->pluginName() )
                currentEntry = i;
       
    }
    
    connect( m_cbias, SIGNAL( biasFactoriesChanged() ), this, SLOT( refreshBiasFactories() ) );
    connect( m_fieldSelection, SIGNAL( activated( int ) ), this, SLOT( selectionChanged( int ) ) );
    connect( m_cbias, SIGNAL( biasChanged( Dynamic::Bias* ) ), this, SIGNAL( biasChanged( Dynamic::Bias* ) ) );

    m_fieldSelection->setCurrentIndex( currentEntry );
    m_weightSelection->setValue( m_cbias->weight() * 100 );
    weightChanged( m_cbias->weight() * 100 );
    // if the custom bias has an entry already loaded, don't reset it.
    // but if not, we create one for ourselves
    if( m_cbias->currentEntry() )
        setCurrentLoadedBiasWidget();
    else
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
    m_cbias->setCurrentEntry( chosen );
    
    setCurrentLoadedBiasWidget();
}

// called when we need to change the selection (like selectionChanged()), but we don' want to re-create the bias--it already exists
// so we just need to set the widget and stuff
void
Dynamic::CustomBiasEntryWidget::setCurrentLoadedBiasWidget()
{
    DEBUG_BLOCK
    if( !m_cbias->currentEntry() ) {
        debug() << "HELP! Assuming bias is properly loaded, but it isn't!";
        return;
    }
    QWidget* config = m_cbias->currentEntry()->configWidget( this );
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
}

void
Dynamic::CustomBiasEntryWidget::weightChanged( int amount )
{
    m_weightLabel->setText( QString().sprintf( "%d%%", amount ) );

    double fval = (double)amount;
    m_cbias->setWeight( fval / 100 );

    emit biasChanged( m_bias );
}

void Dynamic::CustomBiasEntryWidget::refreshBiasFactories()
{
    DEBUG_BLOCK;
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
//                 selectionChanged( m_fieldSelection->currentIndex() );
                setCurrentLoadedBiasWidget();
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

