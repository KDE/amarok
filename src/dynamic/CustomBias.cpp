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

#define DEBUG_PREFIX "CustomBias"

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
    connect( m_weightSelection, SIGNAL( valueChanged( int ) ),
             this, SIGNAL( weightChangedInt(int) ) );

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

    connect( m_fieldSelection, SIGNAL( activated( int ) ),
            this, SLOT( selectionChanged( int ) ) );
    m_fieldSelection->setCurrentIndex( 0 );
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

// CLASS CustomBiasEntry
Dynamic::CustomBiasEntry::CustomBiasEntry( double wieght )
    : m_weight( wieght )
{

}

void
Dynamic::CustomBiasEntry::setWeight(int weight)
{
    m_weight = (double)weight / 100;
}

double
Dynamic::CustomBiasEntry::weight()
{
    return m_weight;
}


// CLASS CustomBias

QList< Dynamic::CustomBiasFactory* > Dynamic::CustomBias::s_biasFactories = QList< Dynamic::CustomBiasFactory* >();
QList< Dynamic::CustomBias* > Dynamic::CustomBias::s_biases = QList< Dynamic::CustomBias* >();
QMap< QString, Dynamic::CustomBias* > Dynamic::CustomBias::s_failedMap = QMap< QString, Dynamic::CustomBias* >();
QMap< QString, QDomElement > Dynamic::CustomBias::s_failedMapXml = QMap< QString, QDomElement >();

        
Dynamic::CustomBias::CustomBias()
    : m_currentEntry( 0 )
    , m_weight( 0 )
{
      //  debug() << "CREATING NEW CUSTOM BIAS WITH NO ARGS :(!!!";
}

Dynamic::CustomBias::CustomBias( Dynamic::CustomBiasEntry* entry, double weight )
    : m_currentEntry( entry )
    , m_weight( weight )
{
    debug() << "CREATING NEW CUSTOM BIAS!!! with:" << entry << weight;
}

Dynamic::CustomBias::~CustomBias()
{
    delete m_currentEntry;
}


PlaylistBrowserNS::BiasWidget*
Dynamic::CustomBias::widget( QWidget* parent )
{
    DEBUG_BLOCK

    debug() << "custombias with weight: " << m_weight << "returning new widget";
    Dynamic::CustomBiasEntryWidget* w = new Dynamic::CustomBiasEntryWidget( this, parent );
    connect( w, SIGNAL( weightChangedInt( int ) ), m_currentEntry, SLOT( setWeight( int ) ) );
    connect( this, SIGNAL( biasFactoriesChanged() ), w, SLOT( refreshBiasFactories() ) );
    return w;
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

QDomElement
Dynamic::CustomBias::xml() const
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

    foreach( QString name, s_failedMap.keys() )
    {
        if( name == entry->pluginName() ) // lazy loading!
        {
            debug() << "found entry loaded without proper custombiasentry. fixing now, with  old weight of" << s_failedMap[ name ]->weight() ;
            //  need to manually set the weight, as we set it on the old widget which is now being thrown away
            Dynamic::CustomBiasEntry* cbe = entry->newCustomBias( s_failedMapXml[ name ], s_failedMap[ name ]->weight() );
            s_failedMap[ name ]->setCurrentEntry( cbe );
            s_failedMap.remove( name );
            s_failedMapXml.remove( name );
        }
    }
    
    foreach( Dynamic::CustomBias* bias, s_biases )
        bias->refreshWidgets();
}


void
Dynamic::CustomBias::removeBiasFactory( Dynamic::CustomBiasFactory* entry )
{
    DEBUG_BLOCK

    if( s_biasFactories.contains( entry ) )
        s_biasFactories.removeAll( entry );

    foreach( Dynamic::CustomBias* bias, s_biases )
        bias->refreshWidgets();
}

Dynamic::CustomBias*
Dynamic::CustomBias::fromXml(QDomElement e)
{
    DEBUG_BLOCK

    QDomElement biasNode = e.firstChildElement( "custombias" );
    if( !biasNode.isNull() )
    {
        debug() << "ok, got a custom bias node. now to create the right bias type";
        QString pluginName = biasNode.attribute( "name", QString() );
        double weight = biasNode.attribute( "weight", QString() ).toDouble();
        if( !pluginName.isEmpty() )
        {
            debug() << "got custom bias type:" << pluginName << "with weight:" << weight;
            foreach( Dynamic::CustomBiasFactory* factory, s_biasFactories )
            {
                if( factory->pluginName() == pluginName )
                {
                    debug() << "found matching bias type! creating :D";
                    return createBias(  factory->newCustomBias( biasNode.firstChild().toElement(), weight ), weight );
                }
            }
            // didn't find a factory for the bias, but we at leasst get a weight, so set that and remember
            debug() << "size of s_failedMap:" << s_failedMap.keys().size();
            Dynamic::CustomBias* b = createBias( 0, weight );
            s_failedMap[ pluginName ] = b;
            s_failedMapXml[ pluginName ] = biasNode.firstChild().toElement();
            return b;
        }
    }
    // couldn't load the xml at all. so instead of crashing we'll return a default custom bias. shouldn't ever be here...
    return createBias();
}


QList< Dynamic::CustomBiasFactory* >
Dynamic::CustomBias::currentFactories()
{
    return s_biasFactories;
}

Dynamic::CustomBias*
Dynamic::CustomBias::createBias()
{
    DEBUG_BLOCK
    Dynamic::CustomBias* newBias = new Dynamic::CustomBias();
    s_biases.append( newBias );
    return newBias;

}

Dynamic::CustomBias*
Dynamic::CustomBias::createBias( Dynamic::CustomBiasEntry* entry, double weight )
{
    DEBUG_BLOCK
    Dynamic::CustomBias* newBias = new Dynamic::CustomBias( entry, weight );
    s_biases.append( newBias );
    return newBias;
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

void Dynamic::CustomBias::refreshWidgets()
{
    emit( biasFactoriesChanged() );
}

#include "CustomBias.moc"
