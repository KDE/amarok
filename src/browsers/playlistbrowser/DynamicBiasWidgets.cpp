/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

// #include "App.h"
#include "Bias.h"
#include "TagMatchBias.h"
#include "PartBias.h"
#include "BiasFactory.h"
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

#include <KComboBox>
#include <KIcon>
#include <KVBox>
#include <klocale.h>

PlaylistBrowserNS::BiasBoxWidget::BiasBoxWidget( Dynamic::BiasPtr bias, QWidget* parent )
    : QWidget( parent )
    , m_mainLayout( 0 )
    , m_bias( bias )
    , m_biasWidget( 0 )
    , m_removable( false )
{
    // setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );

    m_mainLayout = new QHBoxLayout( this );

    biasReplaced( Dynamic::BiasPtr(), bias );
}

void
PlaylistBrowserNS::BiasBoxWidget::setRemovable( bool value )
{
    m_removable = value;
    if( BiasWidget *bw = qobject_cast<BiasWidget*>(m_biasWidget) )
        bw->setRemovable( m_removable );
}

void
PlaylistBrowserNS::BiasBoxWidget::biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias )
{
    Q_UNUSED( oldBias );

    m_bias = newBias;
    m_biasWidget->deleteLater();
    m_biasWidget = 0;

    debug() << "BoxWidget->biasReplaced" << newBias.data();
    if( !newBias )
        return;

    connect( newBias.data(), SIGNAL( replaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ),
             this, SLOT( biasReplaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ) );

    m_biasWidget = newBias->widget( this );
    if( BiasWidget *bw = qobject_cast<BiasWidget*>(m_biasWidget) )
        bw->setRemovable( m_removable );

    m_mainLayout->addWidget( m_biasWidget );
}

void
PlaylistBrowserNS::BiasBoxWidget::paintEvent( QPaintEvent* e )
{
    Q_UNUSED(e);

    // -- paint the background
    QPainter painter(this);

    QStyleOptionViewItemV4 opt;
    opt.widget = this;
    opt.state = QStyle::State_Enabled;
    opt.rect = QRect( 0, 0, width(), height() );

    bool isAlternateRow = rowNumber() % 2;
    if( isAlternateRow )
        opt.features |= QStyleOptionViewItemV2::Alternate;

    /*
    QPalette::ColorGroup cg;
    if ((itemModel->flags(*it) & Qt::ItemIsEnabled) == 0) {
        option.state &= ~QStyle::State_Enabled;
        cg = QPalette::Disabled;
    } else {
        cg = QPalette::Normal;
    }
    opt.palette.setCurrentColorGroup(cg);
    */

    style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, &painter, this );


    // -- paint the fancy splitter line between the items just as the PrettyItemDelegate does
    painter.setRenderHint( QPainter::Antialiasing, true );
    // body = The::svgHandler()->renderSvgWithDividers( "body", width(), height(), "body" );
    painter.drawPixmap( 0, 0,
                        The::svgHandler()->renderSvgWithDividers( "track", width(), height(), "track" ) );

    painter.end();
}

/*
void
PlaylistBrowserNS::BiasBoxWidget::resizeEvent( QResizeEvent* )
{
    emit widgetChanged( this );
}
*/

int
PlaylistBrowserNS::BiasBoxWidget::rowNumber() const
{
    int num = 0;

    if( !m_biasWidget )
        return num;

    LevelBiasWidget *lbw = qobject_cast<LevelBiasWidget*>(parentWidget());
    Dynamic::BiasPtr lastBias = m_bias;
    while( lbw )
    {
        Dynamic::AndBias *aBias = qobject_cast<Dynamic::AndBias*>(lbw);
        if( aBias )
            num += aBias->biases().indexOf( lastBias );

        num++;
        BiasBoxWidget *w = qobject_cast<BiasBoxWidget*>(lbw->parentWidget());
        if( w )
        {
            lastBias = w->m_bias;
            lbw = qobject_cast<LevelBiasWidget*>(lbw->parentWidget());
        }
        else
        {
            lbw = 0;
            break;
        }
    }

    return num;
}

/*

PlaylistBrowserNS::BiasAddWidget::BiasAddWidget( QWidget* parent )
    : BiasBoxWidget( parent )
    , m_addButton( new QToolButton( this ) )
{
    DEBUG_BLOCK

    debug() << "--------------- addwidget";

    QHBoxLayout* mainLayout = new QHBoxLayout( this );
    m_addButton->setIcon( KIcon( "list-add-amarok" ) );
    m_addButton->setToolTip( i18n( "Add a new bias." ) );
    connect( m_addButton, SIGNAL( clicked() ), SLOT( slotClicked() ) );

    QLabel* descLabel = new QLabel( i18n("Add bias") );
    descLabel->setAlignment( Qt::AlignCenter );
    descLabel->setWordWrap( true );

    mainLayout->addWidget( m_addButton );
    mainLayout->setStretchFactor( m_addButton, 0 );
    mainLayout->setAlignment( m_addButton, Qt::AlignLeft | Qt::AlignVCenter );
    mainLayout->addWidget( descLabel );
    mainLayout->setStretchFactor( descLabel, 1 );

    setLayout( mainLayout );
}

// TODO: For some reason this only works with double-click. Also, mouseReleaseEvent() doesn't work at all. Don't know why.
void
PlaylistBrowserNS::BiasAddWidget::mousePressEvent( QMouseEvent* event )
{
    DEBUG_BLOCK

    slotClicked();

    BiasBoxWidget::mousePressEvent( event );
}

void
PlaylistBrowserNS::BiasAddWidget::slotClicked()
{
    DEBUG_BLOCK

    emit addBias();
}
*/

PlaylistBrowserNS::BiasWidget::BiasWidget( Dynamic::BiasPtr b, QWidget* parent )
    : QWidget( parent )
    , m_bias( b )
    , m_removeButton( 0 )
{
    DEBUG_BLOCK

    QHBoxLayout* mainLayout = new QHBoxLayout();
    m_layout = new QFormLayout();
    m_layout->setVerticalSpacing( 0 );
    m_layout->setFormAlignment( Qt::AlignLeft | Qt::AlignTop ); // so that all biases are left aligned
    // m_layout->setFieldGrowthPolicy( QFormLayout::AllNonFixedFieldsGrow );

    // add a remove button if the bias is not in the first level.
    if( qobject_cast<BiasBoxWidget*>(parent) &&
        qobject_cast<BiasWidget*>(parent->parent()) )
    {
        m_removeButton = new QToolButton( this );
        m_removeButton->setIcon( KIcon( "list-remove-amarok" ) );
        m_removeButton->setToolTip( i18n( "Remove this bias." ) );
        m_removeButton->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
        connect( m_removeButton, SIGNAL( clicked() ),
                 this, SLOT( biasRemoved() ) );
    }

    m_biasSelection = new KComboBox( this );
    factoriesChanged();

    connect( Dynamic::BiasFactory::instance(), SIGNAL( changed() ),
             this, SLOT( factoriesChanged() ) );

    connect( m_biasSelection, SIGNAL( activated( int ) ),
             this, SLOT( selectionChanged( int ) ) );

    // connect( m_cbias, SIGNAL( biasChanged( Dynamic::Bias* ) ), this, SIGNAL( biasChanged( Dynamic::Bias* ) ) );

    if( m_removeButton )
    {
        mainLayout->addWidget( m_removeButton, 0/*, Qt::AlignLeft | Qt::AlignTop*/ );
        /*
        mainLayout->setStretchFactor( m_removeButton, 0 );
        mainLayout->setAlignment( m_removeButton, Qt::AlignLeft | Qt::AlignTop );
        */
    }
    mainLayout->addLayout( m_layout, 1 );
    m_layout->addRow( i18n( "Match Type:" ), m_biasSelection );


    setLayout( mainLayout );
}

void
PlaylistBrowserNS::BiasWidget::setRemovable( bool value )
{
    if( m_removeButton )
        m_removeButton->setEnabled( value );
}

void
PlaylistBrowserNS::BiasWidget::factoriesChanged()
{
    m_biasSelection->clear();
    // -- add all the bias types to the list
    QList<Dynamic::AbstractBiasFactory*> factories = Dynamic::BiasFactory::factories();
    debug() << "factories" << factories.count() << "my name:" << m_bias->name();
    int currentIndex = 0;
    for( int i = 0; i <  factories.count(); i++ )
    {
        Dynamic::AbstractBiasFactory* factory = factories.at( i );
    debug() << "factories::" << factory->name();
        m_biasSelection->addItem( factory->i18nName(), QVariant( factory->name() ) );
        if( m_bias && factory->name() == m_bias->name() )
            currentIndex = i;
    }
    m_biasSelection->setCurrentIndex( currentIndex );

}

void
PlaylistBrowserNS::BiasWidget::selectionChanged( int index )
{
    DEBUG_BLOCK
    Q_ASSERT( m_biasSelection );

    debug() << "selection changed to index: " << index;

    QString biasName = m_biasSelection->itemData( index ).toString();
    Dynamic::BiasPtr bias( Dynamic::BiasFactory::fromName( biasName ) );
    if( !bias )
    {
        warning() << "Could not create bias with name:"<<biasName;
        return;
    }

    // TODO: keep the old sub-biases if possible e.g. between AND and OR bias
    m_bias->replace( bias ); // tell the old bias it has just been replaced
}

void
PlaylistBrowserNS::BiasWidget::biasRemoved()
{
    m_bias->replace( Dynamic::BiasPtr() );
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

    if( m_haveWeights )
    {
        /*
           m_weightLabel = new QLabel( " 0%", frame );
           m_weightSelection = new Amarok::Slider( Qt::Horizontal, 100, frame );
           m_weightSelection->setToolTip(
           i18n( "This controls what portion of the playlist should match the criteria" ) );

           QHBoxLayout* sliderLayout = new QHBoxLayout();
           sliderLayout->addWidget( m_weightSelection );
           sliderLayout->addWidget( m_weightLabel );

        m_weightSelection = new Amarok::Slider( Qt::Horizontal, 100, this );
        m_weightSelection->setToolTip(
                                      i18n( "This controls what portion of the playlist should match the criteria" ) );
        m_layout->insertRow( 1, i18n( "Random Proportion:" ), m_weightSelection );

           */
    }

    // -- add an add button to add new widgets
    QHBoxLayout* buttonLayout = new QHBoxLayout( this );

    m_addButton = new QToolButton( this );
    m_addButton->setIcon( KIcon( "list-add-amarok" ) );
    m_addButton->setToolTip( i18n( "Add a new bias." ) );
    connect( m_addButton, SIGNAL( clicked() ),
             this, SLOT( appendBias() ) );

    buttonLayout->addWidget( m_addButton );

    QLabel* label = new QLabel( i18n( "Add bias:") );
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
    m_abias->appendBias( Dynamic::BiasPtr( new Dynamic::RandomBias() ) );
}

void
PlaylistBrowserNS::LevelBiasWidget::biasAppended( Dynamic::BiasPtr bias )
{
    DEBUG_BLOCK;

    // special case for the PartBias. We hide the implicit random sub-bias
    bool specialRandomBias = m_haveWeights &&
        m_sliders.isEmpty() &&
        (qobject_cast<Dynamic::RandomBias*>(bias.data()) != 0);

    // -- add the slider
    if( m_haveWeights )
    {
        Amarok::Slider *slider = new Amarok::Slider( Qt::Horizontal, 100, this );
        if( Dynamic::PartBias *pb = qobject_cast<Dynamic::PartBias*>(m_bias.data()) )
            slider->setValue( pb->weights().last() * 100.0 );

        slider->setToolTip( i18n( "This controls what portion of the playlist should match the criteria" ) );
        m_sliders.append( slider );

        if( specialRandomBias )
            m_layout->insertRow( m_layout->rowCount() - 1,
                                 i18n( "Random Proportion:" ), slider );
        else
            m_layout->insertRow( m_layout->rowCount() - 1,
                                 i18n( "Proportion:" ), slider );

        connect( slider, SIGNAL(valueChanged(int)),
                 SLOT(sliderValueChanged(int)) );
    }

    // -- add the widget
    if( specialRandomBias )
    {
        m_widgets.append( 0 );
    }
    else
    {
        BiasBoxWidget* biasWidget = new BiasBoxWidget( bias, this );
        m_widgets.append( biasWidget );
        m_layout->insertRow( m_layout->rowCount() - 1, biasWidget );
    }

    correctRemovability();
}

void
PlaylistBrowserNS::LevelBiasWidget::biasRemoved( int pos )
{
    BiasBoxWidget* biasWidget = m_widgets.takeAt( pos );

    if( m_haveWeights )
    {
        QSlider *slider = m_sliders.takeAt( pos );

        // - determine the label position
        int rowPtr;
        QFormLayout::ItemRole rolePtr;
        m_layout->getWidgetPosition ( slider, &rowPtr, &rolePtr );
        QLayoutItem* item = m_layout->itemAt( rowPtr, QFormLayout::LabelRole);

        if( item )
            delete item->widget();

        slider->deleteLater();
    }

    biasWidget->deleteLater();

    correctRemovability();
}

void
PlaylistBrowserNS::LevelBiasWidget::biasMoved( int from, int to )
{
    BiasBoxWidget* biasWidget = m_widgets.takeAt( from );
    m_widgets.insert( to, biasWidget );

    if( m_haveWeights )
    {
        QSlider* slider = m_sliders.takeAt( from );
        m_sliders.insert( to, slider );

        // - determine the label position
        int rowPtr;
        QFormLayout::ItemRole rolePtr;
        m_layout->getWidgetPosition ( slider, &rowPtr, &rolePtr );
        QLayoutItem* item = m_layout->itemAt( rowPtr, QFormLayout::LabelRole);

        // -- move the item in the layout
        if( item )
        {
            m_layout->insertRow( to * 2 + 1, biasWidget );
            m_layout->insertRow( to * 2 + 1, item->widget(), slider );
        }
    }
    else
    {
        // -- move the item in the layout
        m_layout->insertRow( to + 1, biasWidget );
    }

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
    m_layout->addRow( i18n( "Match:" ), m_queryWidget );

    syncControlsToBias();

    connect( m_queryWidget, SIGNAL(changed(const MetaQueryWidget::Filter&)),
             SLOT(syncBiasToControls()));
}

void
PlaylistBrowserNS::TagMatchBiasWidget::syncControlsToBias()
{
    /*
    int weight = (int)(m_gbias->weight() * 100.0);
    m_weightSelection->setValue(weight);
    weightChanged(weight); // the widget value might not have changed and thus the signal not fired
    */

    m_queryWidget->setFilter( m_tbias->filter() );
}

void
PlaylistBrowserNS::TagMatchBiasWidget::syncBiasToControls()
{
    m_tbias->setFilter( m_queryWidget->filter() );
}

    /*
void
PlaylistBrowserNS::TagMatchBiasWidget::weightChanged( int ival )
{
    m_weightLabel->setText( QString().sprintf( "%d%%", ival ) );

    double fval = (double)ival / 100.0;
    if( fval != m_gbias->weight() )
    {
        m_gbias->setWeight( fval );
    }
}
    */

#include "DynamicBiasWidgets.moc"
