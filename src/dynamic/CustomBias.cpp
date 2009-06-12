/*****************************************************************************
* copyright            : (C) 2009 Leo Franchi <lfranchi@kde.org>             *
******************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "CustomBias.h"

#include "Debug.h"
#include "SliderWidget.h"

#include <KComboBox>

#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QComboBox>


Dynamic::CustomBiasEntryWidget::CustomBiasEntryWidget(Dynamic::CustomBias* bias, QWidget* parent)
    : PlaylistBrowserNS::BiasWidget( bias, parent )
    , m_bias( bias )
{
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

    m_fieldSelection = new KComboBox( frame );
    m_fieldSelection->setPalette( QApplication::palette() );

    m_layout->addWidget( new QLabel( i18n( "Proportion:" ), frame ), 0, 0 );
    m_layout->addWidget( new QLabel( i18n( "Match Type:" ), frame ), 1, 0 );

    m_layout->addWidget( m_weightSelection, 0, 1 );
    m_layout->addWidget( m_weightLabel, 0, 1 );
    m_layout->addWidget( m_fieldSelection, 1, 1 );

    sliderLayout->addWidget( m_weightSelection );
    sliderLayout->addWidget( m_weightLabel );

    foreach( Dynamic::CustomBiasEntry* entry, m_bias->currentEntries() )
    {
        QVariant data;
        data.setValue( entry );
        m_fieldSelection->addItem( entry->name(), data );
    }

    connect( m_fieldSelection, SIGNAL( currentIndexChanged( int) ),
            this, SLOT( selectionChanged( int ) ) );
    m_fieldSelection->setCurrentIndex( 0 );
    
}


void
Dynamic::CustomBiasEntryWidget::selectionChanged( int index ) // SLOT
{
    DEBUG_BLOCK
    if( !m_fieldSelection )
        return;
    Dynamic::CustomBiasEntry* chosen = m_fieldSelection->itemData( index ).value<  Dynamic::CustomBiasEntry* >();

    if( !chosen )
    {
        debug() << "found a non-CustomBiasEntry in the drop-down..something bad just happened";
        return;
    }

    QWidget* entryConfig = chosen->configWidget();
    if( !entryConfig )
    {
        debug() << "got an invalid config widget from bias type!";
        return;
    }
    // remove last item (old config widget) and add new one
    if( m_layout->count() != 2 )
    {
        debug() << "got more than 2 items in the layout! Whats going on?";
        return;
    }

    m_layout->removeWidget( chosen->configWidget() );
    m_layout->addWidget( entryConfig );
    m_bias->setCurrentEntry( chosen );
}

void
Dynamic::CustomBiasEntryWidget::weightChanged( int amount )
{
    m_bias->setWeight( amount );
}


Dynamic::CustomBias::CustomBias()
    : m_weight( .75 )
{

}

PlaylistBrowserNS::BiasWidget*
Dynamic::CustomBias::widget( QWidget* parent )
{
    DEBUG_BLOCK

    return new Dynamic::CustomBiasEntryWidget( this, parent );
    
}

double
Dynamic::CustomBias::energy( const Meta::TrackList& playlist, const Meta::TrackList& context )
{
    DEBUG_BLOCK

    Q_UNUSED( context );

    double satisfiedCount = m_currentEntry->numTracksThatSatisfy( playlist );

    return  m_weight - (satisfiedCount / (double)playlist.size());
    
}

QDomElement Dynamic::CustomBias::xml() const
{
    DEBUG_BLOCK
    AMAROK_NOTIMPLEMENTED

    return QDomElement();
}

double
Dynamic::CustomBias::reevaluate( double oldEnergy, const Meta::TrackList& oldPlaylist, Meta::TrackPtr newTrack, int newTrackPos, const Meta::TrackList& context )
{
    DEBUG_BLOCK
    Q_UNUSED( context )
    
    double offset = 1.0 / (double)oldPlaylist.size();

    bool prevSatisfied = m_currentEntry->trackSatisfies( oldPlaylist[newTrackPos] );

    if( m_currentEntry->trackSatisfies( newTrack ) && !prevSatisfied )
        return oldEnergy - offset;
    else if( !m_currentEntry->trackSatisfies( newTrack ) && prevSatisfied )
        return oldEnergy + offset;
    else
        return oldEnergy;
    
}


void
Dynamic::CustomBias::registerNewBiasEntry( Dynamic::CustomBiasEntry* entry )
{
    DEBUG_BLOCK
    if( !m_biasEntries.contains( entry ) )
        m_biasEntries.append( entry );
}


void
Dynamic::CustomBias::removeBiasEntry( Dynamic::CustomBiasEntry* entry )
{
    DEBUG_BLOCK

    if( m_biasEntries.contains( entry ) )
        m_biasEntries.removeAll( entry );
}

QList< Dynamic::CustomBiasEntry* >
Dynamic::CustomBias::currentEntries()
{
    return m_biasEntries;
}

void
Dynamic::CustomBias::setCurrentEntry( Dynamic::CustomBiasEntry* entry )
{
    m_currentEntry = entry;
}

void
Dynamic::CustomBias::setWeight( double weight )
{
    m_weight = weight;
}

#include "CustomBias.moc"
