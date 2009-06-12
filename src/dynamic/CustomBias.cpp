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
#include "DynamicModel.h"
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

    m_fieldSelection = new KComboBox( frame );
    m_fieldSelection->setPalette( QApplication::palette() );

    m_layout->addWidget( new QLabel( i18n( "Proportion:" ), frame ), 0, 0 );
    m_layout->addWidget( new QLabel( i18n( "Match Type:" ), frame ), 1, 0 );

    m_layout->addWidget( m_weightSelection, 0, 1 );
    m_layout->addWidget( m_weightLabel, 0, 1 );
    m_layout->addWidget( m_fieldSelection, 1, 1 );

    sliderLayout->addWidget( m_weightSelection );
    sliderLayout->addWidget( m_weightLabel );

    foreach( Dynamic::CustomBiasFactory* entry, m_cbias->currentFactories() )
    {
        QVariant data;
        data.setValue( entry );
        m_fieldSelection->addItem( entry->name(), data );
    }

    connect( m_cbias, SIGNAL( biasFactoriesChanged() ), this, SLOT( reloadBiases() ) );

    connect( m_fieldSelection, SIGNAL( currentIndexChanged( int ) ),
            this, SLOT( selectionChanged( int ) ) );
    m_fieldSelection->setCurrentIndex( 0 );
    selectionChanged( 0 );
    
}


void
Dynamic::CustomBiasEntryWidget::selectionChanged( int index ) // SLOT
{
    DEBUG_BLOCK
    if( !m_fieldSelection )
        return;
    Dynamic::CustomBiasFactory* chosenFactory = m_fieldSelection->itemData( index ).value<  Dynamic::CustomBiasFactory* >();

    if( !chosenFactory )
    {
        debug() << "found a non-CustomBiasFactory in the drop-down..something bad just happened";
        return;
    }

    Dynamic::CustomBiasEntry* chosen = chosenFactory->newCustomBias();
    
    QWidget* entryConfig = chosen->configWidget( this );
    if( !entryConfig )
    {
        debug() << "got an invalid config widget from bias type!";
        return;
    }
    
    // remove last item (old config widget) and old bias and add new one
    if( m_layout->count() == 2 )
    {
        // remove old widget
        
        QLayoutItem* oldW = m_layout->itemAt( 1 );
        m_layout->removeItem( oldW );
        delete oldW;
    }

    entryConfig->setParent( this );
    
    m_layout->addWidget( entryConfig, 2, 0, 1, 3, Qt::AlignCenter );
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

// CLASS CustomBias

QList< Dynamic::CustomBiasFactory* > Dynamic::CustomBias::s_biasFactories = QList< CustomBiasFactory* >();

Dynamic::CustomBias::CustomBias()
    : m_weight( .75 )
    , m_currentEntry( 0 )
{
    DEBUG_BLOCK
    // on new creation, if we have at least one factory just create it
   /* if( s_biasFactories.size() > 0 )
    {
        m_currentEntry = s_biasFactories[ 0 ].newCustomBias();
    } */
}

Dynamic::CustomBias::~CustomBias()
{
    delete m_currentEntry;
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

    double satisfiedCount = 0;
    if( m_currentEntry )
        satisfiedCount = m_currentEntry->numTracksThatSatisfy( playlist );
    else
        warning() << "WHY is there no set type of BIAS?!";
    
    return  m_weight - (satisfiedCount / (double)playlist.size());
    
}

QDomElement Dynamic::CustomBias::xml() const
{
    DEBUG_BLOCK

    if( m_currentEntry )
    {

        QDomDocument doc = PlaylistBrowserNS::DynamicModel::instance()->savedPlaylistDoc();

        QDomElement e = doc.createElement( "bias" );
        e.setAttribute( "type", "custom" );
        QDomElement child = doc.createElement( "custombias" );
        child.setAttribute( "name", m_currentEntry->pluginName() );
        child.setAttribute( "weight", m_weight );

        e.appendChild( child );
        child.appendChild( m_currentEntry->xml( doc ) );
        
        return e;
    } else
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
    {
        debug() << "new satisfies and old doesn't:" << oldEnergy - offset;
        return oldEnergy - offset;
    } else if( !m_currentEntry->trackSatisfies( newTrack ) && prevSatisfied )
    {
        debug() << "new doesn't satisfy and old did:" << oldEnergy + offset;
        return oldEnergy + offset;
    } else
    {
        debug() << "no change:" << oldEnergy;
        return oldEnergy;
    }
}

bool
Dynamic::CustomBias::hasCollectionFilterCapability()
{
    return m_currentEntry && m_currentEntry->hasCollectionFilterCapability();
}


Dynamic::CollectionFilterCapability*
Dynamic::CustomBias::collectionFilterCapability()
{
    if( m_currentEntry )
        return m_currentEntry->collectionFilterCapability();
    else
        return 0;
}

void
Dynamic::CustomBias::registerNewBiasFactory( Dynamic::CustomBiasFactory* entry )
{
    DEBUG_BLOCK
    if( !s_biasFactories.contains( entry ) )
        s_biasFactories.append( entry );

//    emit( biasFactoriesChanged() );
}


void
Dynamic::CustomBias::removeBiasFactory( Dynamic::CustomBiasFactory* entry )
{
    DEBUG_BLOCK

    if( s_biasFactories.contains( entry ) )
        s_biasFactories.removeAll( entry );

//    emit( biasFactoriesChanged() );
}

Dynamic::CustomBias*
Dynamic::CustomBias::fromXml(QDomElement e)
{
    DEBUG_BLOCK

    
    return 0;
}


QList< Dynamic::CustomBiasFactory* >
Dynamic::CustomBias::currentFactories()
{
    return s_biasFactories;
}

void
Dynamic::CustomBias::setCurrentEntry( Dynamic::CustomBiasEntry* entry )
{
    if( m_currentEntry )
        delete m_currentEntry;
    m_currentEntry = entry;
}

Dynamic::CustomBiasEntry*
Dynamic::CustomBias::currentEntry()
{
    return m_currentEntry;
}

void
Dynamic::CustomBias::setWeight( double weight )
{
    m_weight = weight;
    emit weightChanged( m_weight );
}

#include "CustomBias.moc"
