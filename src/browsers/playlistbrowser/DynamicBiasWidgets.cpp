/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "App.h"
#include "Bias.h"
#include "BiasedPlaylist.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "DynamicPlaylist.h"
#include "DynamicBiasModel.h"
#include "DynamicModel.h"
#include "SliderWidget.h"
#include "SvgHandler.h"
#include "widgets/MetaQueryWidget.h"
#include "widgets/kdatecombo.h"

#include <typeinfo>

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPaintEvent>
#include <QPainter>
#include <QSpinBox>
#include <QTimeEdit>
#include <QToolButton>

#include <KComboBox>
#include <KIcon>
#include <KVBox>
#include <klocale.h>

PlaylistBrowserNS::BiasBoxWidget::BiasBoxWidget( QWidget* parent )
    : QWidget( parent ), m_alternate(false)
{
}

void
PlaylistBrowserNS::BiasBoxWidget::paintEvent( QPaintEvent* e )
{
    Q_UNUSED(e)


    // is it showing ?
    QListView* parentList = dynamic_cast<QListView*>(parent());
    if( parentList )
    {
        PlaylistBrowserNS::DynamicBiasModel* model =
            dynamic_cast<PlaylistBrowserNS::DynamicBiasModel*>( parentList->model() );
        if( model )
        {
            QRect rect = parentList->visualRect( model->indexOf( this ) );
            if( rect.x() < 0 || rect.y() < 0 || rect.height() < height() )
            {
                hide();
                return;
            }
        }
    }

    QPainter painter(this);
    painter.setRenderHint( QPainter::Antialiasing, true );

    QPixmap body;

    body = The::svgHandler()->renderSvgWithDividers( "body", width(), height(), "body" );

    painter.drawPixmap( 0, 0, body );

    painter.end();
}

void
PlaylistBrowserNS::BiasBoxWidget::resizeEvent( QResizeEvent* )
{
    emit widgetChanged( this );
}



PlaylistBrowserNS::BiasAddWidget::BiasAddWidget( const QString& caption, const QString& description, QWidget* parent )
    : BiasBoxWidget( parent )
    , m_addButton( new QToolButton( this ) )
{
    DEBUG_BLOCK

    QHBoxLayout* mainLayout = new QHBoxLayout( this );
    m_addButton->setIcon( KIcon( "list-add-amarok" ) );
    m_addButton->setToolTip( i18n( "Add a new bias." ) );
    connect( m_addButton, SIGNAL( clicked() ), SLOT( slotClicked() ) );

    QLabel* descLabel = new QLabel( QString("<b>%1</b><br>%2").arg(caption, description), this );
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




PlaylistBrowserNS::BiasWidget::BiasWidget( Dynamic::Bias* b, QWidget* parent )
    : BiasBoxWidget( parent )
    , m_bias(b)
{
    DEBUG_BLOCK

    QHBoxLayout* mainLayout = new QHBoxLayout( this );

    QToolButton* removeButton = new QToolButton( this );
    removeButton->setIcon( KIcon( "list-remove-amarok" ) );
    removeButton->setToolTip( i18n( "Remove this bias." ) );
    connect( removeButton, SIGNAL( clicked() ), SLOT( biasRemoved() ), Qt::QueuedConnection );

    mainLayout->addWidget( removeButton );
    mainLayout->setStretchFactor( removeButton, 0 );
    mainLayout->setAlignment( removeButton, Qt::AlignLeft | Qt::AlignVCenter );

    setLayout( mainLayout );
}

void
PlaylistBrowserNS::BiasWidget::biasRemoved()
{
    emit biasRemoved( m_bias );
}


PlaylistBrowserNS::BiasGlobalWidget::BiasGlobalWidget(
        Dynamic::GlobalBias* bias, QWidget* parent )
    : BiasWidget( bias, parent )
    , m_gbias( bias )
{
    QFrame *frame = new QFrame( this );
    QFormLayout *layout = new QFormLayout( frame );

    m_weightLabel = new QLabel( " 0%", frame );
    m_weightSelection = new Amarok::Slider( Qt::Horizontal, 100, frame );
    m_weightSelection->setToolTip(
            i18n( "This controls what portion of the playlist should match the criteria" ) );

    QHBoxLayout* sliderLayout = new QHBoxLayout();
    sliderLayout->addWidget( m_weightSelection );
    sliderLayout->addWidget( m_weightLabel );

    m_queryWidget = new MetaQueryWidget( frame );
    m_queryWidget->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding,
                                               QSizePolicy::Preferred ) );
    layout->addRow( i18n( "Proportion:" ), sliderLayout );
    layout->addRow( i18n( "Match:" ), m_queryWidget );

    syncControlsToBias();

    connect( m_weightSelection, SIGNAL(valueChanged(int)),
             SLOT(weightChanged(int)) );
    connect( m_queryWidget, SIGNAL(changed(const MetaQueryWidget::Filter&)),
             SLOT(syncBiasToControls()));

    this->layout()->addWidget( frame );
}

PlaylistBrowserNS::BiasGlobalWidget::~BiasGlobalWidget()
{
}

void
PlaylistBrowserNS::BiasGlobalWidget::syncControlsToBias()
{
    int weight = (int)(m_gbias->weight() * 100.0);
    m_weightSelection->setValue(weight);
    weightChanged(weight); // the widget value might not have changed and thus the signal not fired

    m_queryWidget->setFilter( m_gbias->filter() );
}

void
PlaylistBrowserNS::BiasGlobalWidget::syncBiasToControls()
{
    m_gbias->setFilter( m_queryWidget->filter() );
    m_gbias->setActive( true );

    emit biasChanged( m_gbias );
}

void
PlaylistBrowserNS::BiasGlobalWidget::weightChanged( int ival )
{
    m_weightLabel->setText( QString().sprintf( "%d%%", ival ) );

    double fval = (double)ival / 100.0;
    if( fval != m_gbias->weight() )
    {
        m_gbias->setWeight( fval );

        emit biasChanged( m_gbias );
    }
}



PlaylistBrowserNS::BiasNormalWidget::BiasNormalWidget( Dynamic::NormalBias* bias, QWidget* parent )
    : BiasWidget( bias, parent )
    , m_nbias(bias)
{
    QFrame *frame = new QFrame( this );
    QFormLayout *layout = new QFormLayout( frame );
    frame->setLayout( layout );

    m_scaleLabel = new QLabel( " 0%", frame );
    m_scaleSelection = new Amarok::Slider( Qt::Horizontal, 100, frame );
    m_scaleSelection->setToolTip(
            i18n( "This controls how strictly to match the given value." ) );
    QHBoxLayout* sliderLayout = new QHBoxLayout();
    sliderLayout->addWidget( m_scaleSelection );
    sliderLayout->addWidget( m_scaleLabel );

    m_queryWidget = new MetaQueryWidget( frame, true, true );
    layout->addRow( i18n( "Strictness:" ), sliderLayout );
    layout->addRow( i18n( "Match:" ), m_queryWidget );

    syncControlsToBias();

    connect( m_scaleSelection, SIGNAL(valueChanged(int)),
             SLOT(scaleChanged(int)) );
    connect( m_queryWidget, SIGNAL(changed(const MetaQueryWidget::Filter&)),
             SLOT(syncBiasToControls()));


    this->layout()->addWidget( frame );
}

void
PlaylistBrowserNS::BiasNormalWidget::syncControlsToBias()
{
    int scale = (int)(m_nbias->scale() * 100.0);
    m_scaleSelection->setValue(scale);
    scaleChanged(scale); // the widget value might not have changed and thus the signal not fired

    MetaQueryWidget::Filter filter;
    filter.field    = m_nbias->field();
    filter.numValue = m_nbias->value();

    m_queryWidget->setFilter( filter );

    filter = m_queryWidget->filter();
}

void
PlaylistBrowserNS::BiasNormalWidget::syncBiasToControls()
{
    m_nbias->setField( m_queryWidget->filter().field );
    m_nbias->setValue( m_queryWidget->filter().numValue );
    m_nbias->setScale( m_scaleSelection->value() / 100.0 );
    m_nbias->setActive( true );

    emit biasChanged( m_nbias );
}

void
PlaylistBrowserNS::BiasNormalWidget::scaleChanged( int ival )
{
    m_scaleLabel->setText( QString().sprintf( "%d%%", ival ) );

    double fval = (double)ival / 100.0;
    if( fval != m_nbias->scale() )
    {
        m_nbias->setScale( fval );

        emit biasChanged( m_nbias );
    }
}

#include "DynamicBiasWidgets.moc"

