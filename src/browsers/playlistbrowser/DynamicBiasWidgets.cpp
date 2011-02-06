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
#include "SvgHandler.h"
#include "widgets/MetaQueryWidget.h"

#include <QSlider>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPaintEvent>
#include <QPainter>
#include <QToolButton>

#include <QStyleOption>
#include <QPalette>
#include <QStyle>

#include <KComboBox>
#include <KIcon>
#include <KVBox>
#include <klocale.h>

PlaylistBrowserNS::BiasBoxWidget::BiasBoxWidget( Dynamic::BiasPtr bias, QWidget* parent )
    : QWidget( parent )
    , m_mainLayout( 0 )
    , m_bias( bias )
    , m_biasWidget( 0 )
    , m_removeButton( 0 )
    , m_hover( false )
{
    setAutoFillBackground( true );

    m_mainLayout = new QHBoxLayout( this );
    QMargins margins = m_mainLayout->contentsMargins();
    margins.setLeft( 0 );
    margins.setRight( 0 );
    m_mainLayout->setContentsMargins( margins );

    // add a remove button if the bias is not in the first level.
    if( qobject_cast<BiasWidget*>(parentWidget()) )
    {
        m_removeButton = new QToolButton( this );
        m_removeButton->setIcon( KIcon( "list-remove-amarok" ) );
        m_removeButton->setToolTip( i18n( "Remove this bias." ) );
        m_removeButton->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
        connect( m_removeButton, SIGNAL( clicked() ),
                 this, SLOT( biasRemoved() ) );
    }

    if( m_removeButton )
    {
        m_mainLayout->addWidget( m_removeButton, 0 );
    }

    biasReplaced( Dynamic::BiasPtr(), bias );
}

void
PlaylistBrowserNS::BiasBoxWidget::setRemovable( bool value )
{
    if( m_removeButton )
        m_removeButton->setEnabled( value );
}

void
PlaylistBrowserNS::BiasBoxWidget::biasRemoved()
{
    m_bias->replace( Dynamic::BiasPtr() );
}


void
PlaylistBrowserNS::BiasBoxWidget::biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias )
{
    Q_UNUSED( oldBias );

    if( m_biasWidget )
    {
        m_biasWidget->deleteLater();
        m_biasWidget = 0;
    }

    debug() << "BoxWidget->biasReplaced" << newBias.data();
    m_bias = newBias;
    if( !newBias )
        return;

    connect( newBias.data(), SIGNAL( replaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ),
             this, SLOT( biasReplaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ) );

    m_biasWidget = newBias->widget( this );

    m_mainLayout->addWidget( m_biasWidget, 1 );
}

void
PlaylistBrowserNS::BiasBoxWidget::paintEvent( QPaintEvent *event )
{
    Q_UNUSED(event);

    // -- paint the background
    QPainter painter(this);

    QStyleOptionViewItemV4 option;
    option.widget = this;
    option.state = QStyle::State_Enabled;
    option.rect = QRect( 0, 0, width(), height() );

    bool isAlternateRow = rowNumber() % 2;
    if( isAlternateRow )
        option.features |= QStyleOptionViewItemV2::Alternate;

    // -- the hover color is an indication that you can move the bias
    // a bias can't be moved if it can't be removed
    if( m_hover && m_removeButton && m_removeButton->isEnabled() )
        option.state |= QStyle::State_MouseOver;
    else
        option.state &= ~QStyle::State_MouseOver;

    style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, &painter, this );

    // -- paint the fancy splitter line between the items just as the PrettyItemDelegate does
    painter.setRenderHint( QPainter::Antialiasing, true );
    painter.drawPixmap( 0, 0,
                        The::svgHandler()->renderSvgWithDividers( "track", width(), height(), "track" ) );

    painter.end();
}

void
PlaylistBrowserNS::BiasBoxWidget::enterEvent( QEvent *event )
{
    Q_UNUSED(event);
    m_hover = true;
    update();
}

void
PlaylistBrowserNS::BiasBoxWidget::leaveEvent( QEvent *event )
{
    Q_UNUSED(event);
    m_hover = false;
    update();
}

int
PlaylistBrowserNS::BiasBoxWidget::rowNumber() const
{
    int num = 0;

    if( !m_biasWidget )
        return num;

    // usually we have a BiasBoxWidget inside a LevelBiasWidget inside a BiasBoxWidget.

    BiasBoxWidget *bbw = const_cast<BiasBoxWidget*>(this);
    LevelBiasWidget *lbw = qobject_cast<LevelBiasWidget*>(parentWidget());
    while( lbw )
    {
        num++;
        num += lbw->widgets().indexOf( bbw );

        bbw = qobject_cast<BiasBoxWidget*>(lbw->parentWidget());
        if( bbw )
            lbw = qobject_cast<LevelBiasWidget*>(bbw->parentWidget());
        else
            lbw = 0;
    }

    return num;
}

// ------- BiasWidget ----------

PlaylistBrowserNS::BiasWidget::BiasWidget( Dynamic::BiasPtr b, QWidget* parent )
    : QWidget( parent )
    , m_bias( b )
    , m_customWidget( 0 )
{
    m_layout = new QFormLayout();
    // m_layout->setVerticalSpacing( 0 );
    m_layout->setFormAlignment( Qt::AlignLeft | Qt::AlignTop ); // so that all biases are left aligned

    QMargins margins = m_layout->contentsMargins();
    margins.setLeft( 0 );
    margins.setRight( 0 );
    m_layout->setContentsMargins( margins );

    // m_layout->setFieldGrowthPolicy( QFormLayout::AllNonFixedFieldsGrow );

    // connect( m_cbias, SIGNAL( biasChanged( Dynamic::Bias* ) ), this, SIGNAL( biasChanged( Dynamic::Bias* ) ) );
    m_biasSelection = new KComboBox( this );
    factoriesChanged();

    m_layout->addRow( i18nc("Bias selection label in bias view. Try to keep below 15 characters or abbreviate", "Match Type:" ), m_biasSelection );

    connect( Dynamic::BiasFactory::instance(), SIGNAL( changed() ),
             this, SLOT( factoriesChanged() ) );

    connect( m_biasSelection, SIGNAL( activated( int ) ),
             this, SLOT( selectionChanged( int ) ) );

    setLayout( m_layout );
}

void
PlaylistBrowserNS::BiasWidget::setCustomWidget( const QString &label, QWidget* widget )
{
    // -- remove the old custom widget
    if( m_customWidget )
    {
        // - determine the label position
        int rowPtr;
        QFormLayout::ItemRole rolePtr;
        m_layout->getWidgetPosition ( m_customWidget, &rowPtr, &rolePtr );
        QLayoutItem* item = m_layout->itemAt( rowPtr, QFormLayout::LabelRole);

        if( item )
            delete item->widget();

        m_customWidget->deleteLater();
    }

    m_customWidget = widget;
    if( !widget )
        return;

    m_layout->insertRow( 0, label, m_customWidget );
}


void
PlaylistBrowserNS::BiasWidget::factoriesChanged()
{
    m_biasSelection->clear();

    // -- add all the bias types to the list
    QList<Dynamic::AbstractBiasFactory*> factories = Dynamic::BiasFactory::factories();
    for( int i = 0; i <  factories.count(); i++ )
    {
        Dynamic::AbstractBiasFactory* factory = factories.at( i );
        m_biasSelection->addItem( factory->i18nName(), QVariant( factory->name() ) );

        // -- set the current index if we have found our own factory
        if( m_bias && factory->name() == m_bias->name() )
        {
            m_biasSelection->setCurrentIndex( i );
            // while we are at it: set a tool tip
            setToolTip( factory->i18nDescription() );
        }
    }
}

void
PlaylistBrowserNS::BiasWidget::selectionChanged( int index )
{
    Q_ASSERT( m_biasSelection );

    QString biasName = m_biasSelection->itemData( index ).toString();

    Dynamic::BiasPtr oldBias( m_bias );
    Dynamic::BiasPtr newBias( Dynamic::BiasFactory::fromName( biasName ) );
    if( !newBias )
    {
        warning() << "Could not create bias with name:"<<biasName;
        return;
    }

    m_bias->replace( newBias ); // tell the old bias it has just been replaced

    // -- if the new bias is AndBias, try to add the old biase(s) into it
    Dynamic::AndBias *oldABias = qobject_cast<Dynamic::AndBias*>(oldBias.data());
    Dynamic::AndBias *newABias = qobject_cast<Dynamic::AndBias*>(newBias.data());
    if( newABias ) {
        if( oldABias ) {
            for( int i = 0; i < oldABias->biases().count(); i++ )
            {
                // skip the default random bias of the PartBias
                if( i > 0 || !qobject_cast<Dynamic::PartBias*>(oldABias) )
                    newABias->appendBias( oldABias->biases()[i] );
            }
        }
        else
        {
            newABias->appendBias( oldBias );
        }
    }
}

// -------- LevelBiasWidget -----------


PlaylistBrowserNS::LevelBiasWidget::LevelBiasWidget( Dynamic::AndBias* bias,
                                                     bool haveWeights,
                                                     QWidget* parent )
    : BiasWidget( Dynamic::BiasPtr(bias), parent )
    , m_haveWeights( haveWeights )
    , m_inSignal( false )
    , m_abias( bias )
{
    connect( bias, SIGNAL( biasAppended( Dynamic::BiasPtr ) ),
             this, SLOT( biasAppended( Dynamic::BiasPtr ) ) );

    connect( bias, SIGNAL( biasRemoved( int ) ),
             this, SLOT( biasRemoved( int ) ) );

    connect( bias, SIGNAL( biasMoved( int, int ) ),
             this, SLOT( biasMoved( int, int ) ) );

    if( m_haveWeights )
    {
        connect( this, SIGNAL( biasWeightChanged( int, qreal ) ),
                 bias, SLOT( changeBiasWeight( int, qreal ) ) );
        connect( bias, SIGNAL( weightsChanged() ),
                 this, SLOT( biasWeightsChanged() ) );
    }

    QMargins margins = contentsMargins();
    margins.setLeft( 0 ); // margin to the left of the add/remove buttons
    margins.setRight( 0 );
    setContentsMargins( margins );

    // -- add an add button to add new widgets
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    // buttonLayout->addSpacing( style()->pixelMetric( QStyle::PM_LayoutLeftMargin ) );

    m_addButton = new QToolButton( this );
    m_addButton->setIcon( KIcon( "list-add-amarok" ) );
    m_addButton->setToolTip( i18n( "Add a new bias." ) );
    connect( m_addButton, SIGNAL( clicked() ),
             this, SLOT( appendBias() ) );

    buttonLayout->addWidget( m_addButton );

    QLabel* label = new QLabel( i18n( "Add bias") );
    buttonLayout->addWidget( label );

    m_layout->addRow( buttonLayout );


    // -- add all sub-bias widgets
    foreach( Dynamic::BiasPtr bias, m_abias->biases() )
    {
        biasAppended( bias );
    }
}

void
PlaylistBrowserNS::LevelBiasWidget::appendBias()
{
    m_abias->appendBias( Dynamic::BiasPtr( new Dynamic::TagMatchBias() ) );
}

void
PlaylistBrowserNS::LevelBiasWidget::biasAppended( Dynamic::BiasPtr bias )
{
    // special case for the PartBias. We hide the implicit random sub-bias
    bool specialRandomBias = m_haveWeights &&
        m_sliders.isEmpty() &&
        (qobject_cast<Dynamic::RandomBias*>(bias.data()) != 0);

    Amarok::Slider *slider = 0;
    BiasBoxWidget *biasBoxWidget = 0;
    if( m_haveWeights )
    {
        slider = new Amarok::Slider( Qt::Horizontal, 100, this );

        if( Dynamic::PartBias *pb = qobject_cast<Dynamic::PartBias*>(m_bias.data()) )
        {
            slider->setValue( pb->weights()[ pb->biases().indexOf( bias ) ] * 100.0 );
        }

        slider->setToolTip( i18n( "This controls what portion of the playlist should match the criteria" ) );

        connect( slider, SIGNAL(valueChanged(int)),
                 SLOT(sliderValueChanged(int)) );
    }

    if( specialRandomBias )
    {
        // -- add the slider
        m_layout->insertRow( m_layout->rowCount() - 1, i18n( "Random Proportion:" ), slider );
    }
    else
    {
        // -- add the widget (with slider)
        biasBoxWidget = new BiasBoxWidget( bias, this );
        if( slider && biasBoxWidget->biasWidget() )
            biasBoxWidget->biasWidget()->setCustomWidget( i18n( "Proportion:" ), slider );
        m_layout->insertRow( m_layout->rowCount() - 1, biasBoxWidget );
    }
    m_sliders.append( slider );
    m_widgets.append( biasBoxWidget );

    correctRemovability();
}

void
PlaylistBrowserNS::LevelBiasWidget::biasRemoved( int pos )
{
    m_sliders.takeAt( pos );
    BiasBoxWidget* biasWidget = m_widgets.takeAt( pos );

    biasWidget->deleteLater();

    correctRemovability();
}

void
PlaylistBrowserNS::LevelBiasWidget::biasMoved( int from, int to )
{
    QSlider* slider = m_sliders.takeAt( from );
    m_sliders.insert( to, slider );
    BiasBoxWidget* biasWidget = m_widgets.takeAt( from );
    m_widgets.insert( to, biasWidget );

    // -- move the item in the layout
    m_layout->insertRow( to + 1, biasWidget );

    correctRemovability();
}

void
PlaylistBrowserNS::LevelBiasWidget::sliderValueChanged( int val )
{
    // protect agains recursion
    if( m_inSignal )
        return;

    for( int i = 0; i < m_sliders.count(); i++ )
    {
        if( m_sliders.at(i) == sender() )
            emit biasWeightChanged( i, qreal(val) / 100.0 );
    }
}

void
PlaylistBrowserNS::LevelBiasWidget::biasWeightsChanged()
{
    // protect agains recursion
    if( m_inSignal )
        return;
    m_inSignal = true;

    if( Dynamic::PartBias *pb = qobject_cast<Dynamic::PartBias*>(m_bias.data()) )
    {
        QList<qreal> weights = pb->weights();
        for( int i = 0; i < weights.count(); i++ )
            m_sliders.at(i)->setValue( weights.at(i) * 100.0 );
    }

    m_inSignal = false;
}

void
PlaylistBrowserNS::LevelBiasWidget::correctRemovability()
{
    for( int i = 0; i < m_widgets.count(); i++ )
        if( m_widgets.at( i ) )
            m_widgets.at( i )->setRemovable( m_widgets.count() > 1 );
}


// ---------- TagMatchBias --------


PlaylistBrowserNS::TagMatchBiasWidget::TagMatchBiasWidget( Dynamic::TagMatchBias* bias,
                                                           QWidget* parent )
    : BiasWidget( Dynamic::BiasPtr(bias), parent )
    , m_tbias( bias )
{
    m_queryWidget = new MetaQueryWidget( this );
    m_queryWidget->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding,
                                               QSizePolicy::Preferred ) );
    m_layout->addRow( i18nc("Tag Match Bias selection label in bias view. Try to keep below 15 characters or abbreviate", "Match:" ), m_queryWidget );

    syncControlsToBias();

    connect( m_queryWidget, SIGNAL(changed(const MetaQueryWidget::Filter&)),
             SLOT(syncBiasToControls()));
}

void
PlaylistBrowserNS::TagMatchBiasWidget::syncControlsToBias()
{
    m_queryWidget->setFilter( m_tbias->filter() );
}

void
PlaylistBrowserNS::TagMatchBiasWidget::syncBiasToControls()
{
    m_tbias->setFilter( m_queryWidget->filter() );
}


#include "DynamicBiasWidgets.moc"
