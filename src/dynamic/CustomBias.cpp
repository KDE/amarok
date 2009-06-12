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

#include <QFrame>
#include <QVBoxLayout>
#include <QComboBox>

Dynamic::CustomBias::CustomBias()
    : m_biasTypes( 0 )
{

}

PlaylistBrowserNS::BiasWidget*
Dynamic::CustomBias::widget( QWidget* parent )
{
    DEBUG_BLOCK
    // create widget with combobox, on selection, update with
    // delegate widget.
    QFrame* frame = new QFrame( parent );
    QVBoxLayout* m_layout = new QVBoxLayout( frame );

    frame->setLayout( m_layout );

    // build combobox
    m_biasTypes = new QComboBox( parent );
    m_layout->addWidget( m_biasTypes );
    foreach( Dynamic::CustomBiasEntry* entry, m_biasEntries )
    {
        QVariant data;
        data.setValue( entry );
        m_biasTypes->addItem( entry->name(), data );
    }
    connect( m_biasTypes, SIGNAL( currentIndexChanged( int ) ), SLOT( selectionChanged( int ) ) );
    
}

void
Dynamic::CustomBias::selectionChanged( int index ) // SLOT
{
    DEBUG_BLOCK
    if( !m_biasTypes )
        return;
    Dynamic::CustomBiasEntry* chosen = m_biasTypes->itemData( index ).value<  Dynamic::CustomBiasEntry* >();

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
    m_active = chosen;
}



double
Dynamic::CustomBias::energy( const Meta::TrackList& playlist, const Meta::TrackList& context )
{
    DEBUG_BLOCK
}

QDomElement Dynamic::CustomBias::xml() const
{
    DEBUG_BLOCK
    AMAROK_NOTIMPLEMENTED
}

double
Dynamic::CustomBias::reevaluate( double oldEnergy, const Meta::TrackList& oldPlaylist, Meta::TrackPtr newTrack, int newTrackPos, const Meta::TrackList& context )
{
    DEBUG_BLOCK
    
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