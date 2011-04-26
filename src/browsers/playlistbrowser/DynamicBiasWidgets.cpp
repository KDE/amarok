/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2010,2011 Ralf Engels <ralf-engels@gmx.de>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DynamicBiasWidgets.h"

#include "Bias.h"
#include "BiasFactory.h"
#include "biases/TagMatchBias.h"
#include "biases/PartBias.h"
#include "core/support/Debug.h"
#include "SliderWidget.h"
#include "widgets/MetaQueryWidget.h"

#include <QLabel>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>

#include <KComboBox>
#include <KIcon>
#include <KVBox>
#include <klocale.h>

PlaylistBrowserNS::BiasDialog::BiasDialog( Dynamic::BiasPtr bias, QWidget* parent )
    : QDialog( parent )
    , m_mainLayout( 0 )
    , m_biasLayout( 0 )
    , m_descriptionLabel( 0 )
    , m_biasWidget( 0 )
    , m_bias( bias )
{
    setWindowTitle( i18nc( "Bias dialog window title", "Edit bias" ) );
    m_mainLayout = new QVBoxLayout( this );

    // -- the bias selection combo
    QLabel* selectionLabel = new QLabel( i18nc("Bias selection label in bias view.", "Match Type:" ) );
    m_biasSelection = new KComboBox();
    QHBoxLayout *selectionLayout = new QHBoxLayout();
    selectionLabel->setBuddy( m_biasSelection );
    selectionLayout->addWidget( selectionLabel );
    selectionLayout->addWidget( m_biasSelection );
    selectionLayout->addStretch( 1 );
    m_mainLayout->addLayout( selectionLayout );

    // -- bias itself
    m_descriptionLabel = new QLabel( "" );
    m_mainLayout->addWidget( m_descriptionLabel );

    m_biasLayout = new QVBoxLayout();
    m_mainLayout->addLayout( m_biasLayout );

    // -- button box
    QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok );
    m_mainLayout->addWidget( buttonBox );

    factoriesChanged();
    biasReplaced( Dynamic::BiasPtr(), bias );

    connect( Dynamic::BiasFactory::instance(), SIGNAL( changed() ),
             this, SLOT( factoriesChanged() ) );
    connect( m_biasSelection, SIGNAL( activated( int ) ),
             this, SLOT( selectionChanged( int ) ) );
    connect(buttonBox, SIGNAL(accepted()),
            this, SLOT(accept()));
}

PlaylistBrowserNS::BiasDialog::~BiasDialog()
{ }

void
PlaylistBrowserNS::BiasDialog::factoriesChanged()
{
    m_biasSelection->clear();

    disconnect( Dynamic::BiasFactory::instance(), SIGNAL( changed() ),
                this, SLOT( factoriesChanged() ) );

    // -- add all the bias types to the list
    bool factoryFound = false;
    QList<Dynamic::AbstractBiasFactory*> factories = Dynamic::BiasFactory::factories();
    for( int i = 0; i <  factories.count(); i++ )
    {
        Dynamic::AbstractBiasFactory* factory = factories.at( i );
        m_biasSelection->addItem( factory->i18nName(), QVariant( factory->name() ) );

        // -- set the current index if we have found our own factory
        if( m_bias && factory->name() == m_bias->name() )
        {
            factoryFound = true;
            m_biasSelection->setCurrentIndex( i );
            m_descriptionLabel->setText( factory->i18nDescription() );
        }
    }

    // -- In cases of replacement bias
    if( !factoryFound )
    {
        m_biasSelection->addItem( m_bias->name() );
        m_biasSelection->setCurrentIndex( m_biasSelection->count() );
        m_descriptionLabel->setText( i18n( "This bias is a replacement for another bias\n"
                                         "which is currently not loaded or deactivated.\n"
                                         "The original bias name was %1." ).arg( m_bias->name() ) );
    }

    connect( Dynamic::BiasFactory::instance(), SIGNAL( changed() ),
             this, SLOT( factoriesChanged() ) );
}

void
PlaylistBrowserNS::BiasDialog::selectionChanged( int index )
{
    DEBUG_BLOCK;
    Q_ASSERT( m_biasSelection );

    QString biasName = m_biasSelection->itemData( index ).toString();

    Dynamic::BiasPtr oldBias( m_bias );
    Dynamic::BiasPtr newBias( Dynamic::BiasFactory::fromName( biasName ) );
    if( !newBias )
    {
        warning() << "Could not create bias with name:"<<biasName;
        return;
    }

debug() << "replace bias" << oldBias->toString() << "with" << newBias->toString();
    m_bias->replace( newBias ); // tell the old bias it has just been replaced
debug() << "replaced";

    // -- if the new bias is AndBias, try to add the old biase(s) into it
    Dynamic::AndBias *oldABias = qobject_cast<Dynamic::AndBias*>(oldBias.data());
    Dynamic::AndBias *newABias = qobject_cast<Dynamic::AndBias*>(newBias.data());
    if( newABias ) {
        if( oldABias ) {
            for( int i = 0; i < oldABias->biases().count(); i++ )
            {
                newABias->appendBias( oldABias->biases()[i] );
            }
        }
        else
        {
            newABias->appendBias( oldBias );
        }
    }
}

void
PlaylistBrowserNS::BiasDialog::biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias )
{
    Q_UNUSED( oldBias );

    if( m_biasWidget )
    {
        m_biasLayout->removeWidget( m_biasWidget );
        m_biasWidget->deleteLater();
        m_biasWidget = 0;
    }

    m_bias = newBias;
    if( !newBias )
        return;

    connect( newBias.data(), SIGNAL( replaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ),
             this, SLOT( biasReplaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ) );

    m_biasWidget = newBias->widget( 0 );
    if( !m_biasWidget )
        m_biasWidget = new QLabel( i18n("This bias has no settings") );
    m_biasLayout->addWidget( m_biasWidget );

    factoriesChanged(); // update the bias description and select the new combo entry
}


// -------- PartBiasWidget -----------


PlaylistBrowserNS::PartBiasWidget::PartBiasWidget( Dynamic::PartBias* bias, QWidget* parent )
    : QWidget( parent )
    , m_inSignal( false )
    , m_bias( bias )
{
    connect( bias, SIGNAL( biasAppended( Dynamic::BiasPtr ) ),
             this, SLOT( biasAppended( Dynamic::BiasPtr ) ) );

    connect( bias, SIGNAL( biasRemoved( int ) ),
             this, SLOT( biasRemoved( int ) ) );

    connect( bias, SIGNAL( biasMoved( int, int ) ),
             this, SLOT( biasMoved( int, int ) ) );

    connect( bias, SIGNAL( weightsChanged() ),
             this, SLOT( biasWeightsChanged() ) );

    m_layout = new QGridLayout( this );

    // -- add all sub-bias widgets
    foreach( Dynamic::BiasPtr bias, m_bias->biases() )
    {
        biasAppended( bias );
    }
}

void
PlaylistBrowserNS::PartBiasWidget::appendBias()
{
    m_bias->appendBias( Dynamic::BiasPtr( new Dynamic::TagMatchBias() ) );
}

void
PlaylistBrowserNS::PartBiasWidget::biasAppended( Dynamic::BiasPtr bias )
{
    int index = m_bias->biases().indexOf( bias );

    Amarok::Slider* slider = 0;
    slider = new Amarok::Slider( Qt::Horizontal, 100 );
    slider->setValue( m_bias->weights()[ m_bias->biases().indexOf( bias ) ] * 100.0 );
    slider->setToolTip( i18n( "This controls what portion of the playlist should match the criteria" ) );
    connect( slider, SIGNAL(valueChanged(int)), SLOT(sliderValueChanged(int)) );

    QLabel* label = new QLabel( bias->toString() );

    m_sliders.append( slider );
    m_widgets.append( label );
    // -- add the widget (with slider)
    m_layout->addWidget( slider, index, 0 );
    m_layout->addWidget( label, index, 1 );
}

void
PlaylistBrowserNS::PartBiasWidget::biasRemoved( int pos )
{
    m_layout->takeAt( pos * 2 );
    m_layout->takeAt( pos * 2 );
    m_sliders.takeAt( pos )->deleteLater();
    m_widgets.takeAt( pos )->deleteLater();
}

void
PlaylistBrowserNS::PartBiasWidget::biasMoved( int from, int to )
{
    QSlider* slider = m_sliders.takeAt( from );
    m_sliders.insert( to, slider );

    QWidget* widget = m_widgets.takeAt( from );
    m_widgets.insert( to, widget );

    // -- move the item in the layout
    // TODO
    /*
    m_layout->insertWidget( to * 2, slider );
    m_layout->insertWidget( to * 2 + 1, widget );
    */
}

void
PlaylistBrowserNS::PartBiasWidget::sliderValueChanged( int val )
{
    DEBUG_BLOCK;
    // protect agains recursion
    if( m_inSignal )
        return;

    for( int i = 0; i < m_sliders.count(); i++ )
    {
        if( m_sliders.at(i) == sender() )
            m_bias->changeBiasWeight( i, qreal(val) / 100.0 );
    }
}

void
PlaylistBrowserNS::PartBiasWidget::biasWeightsChanged()
{
    DEBUG_BLOCK;
    // protect agains recursion
    if( m_inSignal )
        return;

    m_inSignal = true;

    QList<qreal> weights = m_bias->weights();
    for( int i = 0; i < weights.count() && i < m_sliders.count(); i++ )
        m_sliders.at(i)->setValue( weights.at(i) * 100.0 );

    m_inSignal = false;
}


#include "DynamicBiasWidgets.moc"
