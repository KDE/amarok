/***************************************************************************
 * copyright         : (C) 2008 Daniel Caleb Jones <danielcjones@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "App.h"
#include "Debug.h"
#include "DynamicBiasWidgets.h"
#include "QueryMaker.h"
#include "SliderWidget.h"
#include "SvgTinter.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QToolButton>

#include <KComboBox>
#include <KIcon>
#include <KToolBar>
#include <KVBox>

PlaylistBrowserNS::BiasBoxWidget::BiasBoxWidget( QWidget* parent )
    : QWidget( parent )
{
}

void
PlaylistBrowserNS::BiasBoxWidget::paintEvent( QPaintEvent* e )
{
    Q_UNUSED(e)

    QPainter painter(this);
    painter.setRenderHint( QPainter::Antialiasing, true );

    QColor fill = App::instance()->palette().base().color();
    fill.setAlpha( 96 );

    QColor border = 
        The::svgTinter()->blendColors(
                App::instance()->palette().window().color(), "#000000", 90 );

    painter.setBrush( QBrush(fill) );
    painter.setPen( QPen( QBrush(border), 1 ) );

    //int ry = height() / 10;
    //int rx = width() / 10;
    //painter.drawRoundRect( QRect( QPoint(0,0), size() ), ry, rx );

    painter.drawRect( QRect( QPoint(0,0), size() ) );

    painter.end();
}



PlaylistBrowserNS::BiasAddWidget::BiasAddWidget( QWidget* parent )
    : BiasBoxWidget( parent )
{
    QHBoxLayout* mainLayout = new QHBoxLayout( this );

    QWidget* m_addToolbarWidget = new QWidget( this );
    m_addToolbarWidget->setFixedSize( 30, 30 );

    m_addToolbar = new KToolBar( m_addToolbarWidget );
    m_addToolbar->setFixedHeight( 30 );
    //m_addToolbar->setContentsMargins( 0, 0, 0, 0 );
    m_addToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_addToolbar->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    m_addToolbar->setIconDimensions( 20 );
    m_addToolbar->setMovable( false );
    m_addToolbar->setFloatable ( false );

    m_addButton = new QToolButton( m_addToolbar );
    m_addButton->setIcon( KIcon( "list-add-amarok" ) );
    m_addToolbar->addWidget( m_addButton );
    m_addToolbar->adjustSize();

    m_addLabel = new QLabel( "New Bias", this );
    m_addLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    QFont font;
    font.setPointSize( 16 );
    m_addLabel->setFont( font );

    mainLayout->addWidget( m_addToolbarWidget );
    mainLayout->setStretchFactor( m_addToolbarWidget, 0 );
    mainLayout->setAlignment( m_addToolbarWidget, Qt::AlignLeft | Qt::AlignVCenter );
    mainLayout->addWidget( m_addLabel );
    mainLayout->setStretchFactor( m_addLabel, 1 );

    setLayout( mainLayout );
}




PlaylistBrowserNS::BiasWidget::BiasWidget( Dynamic::Bias* b, QWidget* parent )
    : BiasBoxWidget( parent )
{
    QHBoxLayout* hLayout = new QHBoxLayout( this );

    QWidget* removeToolbarWidget = new QWidget(this);
    removeToolbarWidget->setMinimumSize( 30, 30 );
    //removeToolbarWidget->setContentsMargins( 0, 0, 0, 0 );

    m_removeToolbar = new KToolBar(removeToolbarWidget);
    //m_removeToolbar->setFixedHeight( 30 );
    //m_removeToolbar->setContentsMargins( 0, 0, 0, 0 );
    m_removeToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_removeToolbar->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    m_removeToolbar->setIconDimensions( 20 );
    m_removeToolbar->setMovable( false );
    m_removeToolbar->setFloatable ( false );

    QToolButton* removeButton = new QToolButton( m_removeToolbar );
    removeButton->setIcon( KIcon( "list-remove-amarok" ) );
    m_removeToolbar->addWidget( removeButton );
    m_removeToolbar->adjustSize();


    m_mainLayout = new KVBox( this );

    hLayout->addWidget( removeToolbarWidget );
    hLayout->setStretchFactor( removeToolbarWidget, 0 );
    hLayout->setAlignment( removeToolbarWidget, Qt::AlignLeft | Qt::AlignVCenter );

    hLayout->addWidget( m_mainLayout );

    setLayout( hLayout );
}


PlaylistBrowserNS::BiasGlobalWidget::BiasGlobalWidget(
        Dynamic::GlobalBias* bias, QWidget* parent )
    : BiasWidget( bias, parent ), m_bias( bias )
{
    QGridLayout* controlLayout = new QGridLayout( m_mainLayout );
    QFrame* controlFrame = new QFrame( m_mainLayout );
    controlFrame->setLayout( controlLayout );

    QHBoxLayout* sliderLayout = new QHBoxLayout( controlFrame );
    controlLayout->addLayout( sliderLayout, 0, 1 );

    m_weightLabel = new QLabel( " 0%", controlFrame );
    m_weightSelection = new Amarok::Slider( Qt::Horizontal, controlFrame, 100 );
    connect( m_weightSelection, SIGNAL(valueChanged(int)),
            this, SLOT(weightChanged(int)) );

    m_fieldSelection = new KComboBox( controlFrame );

    controlLayout->addWidget( new QLabel( "Proportion:", controlFrame ), 0, 0 );
    controlLayout->addWidget( new QLabel( "Match:", controlFrame ), 1, 0 );
    controlLayout->addWidget( new QLabel( "With:", controlFrame ), 2, 0 );

    controlLayout->addWidget( m_weightSelection, 0, 1 );
    controlLayout->addWidget( m_weightLabel, 0, 1 );
    controlLayout->addWidget( m_fieldSelection, 1, 1 );

    sliderLayout->addWidget( m_weightSelection );
    sliderLayout->addWidget( m_weightLabel );

    popuplateFieldSection();
}

void
PlaylistBrowserNS::BiasGlobalWidget::popuplateFieldSection()
{
    m_fieldSelection->addItem( "Artist", QueryMaker::valArtist );
    m_fieldSelection->addItem( "Title",  QueryMaker::valTitle );
    // TODO: add all these, once there are widgets for them all
        
}

void
PlaylistBrowserNS::BiasGlobalWidget::weightChanged( int ival )
{
    double fval = (double)ival;
    m_weightLabel->setText( QString().sprintf( "%2.0f%%", fval ) );

    // TODO: set the bias weight
}


#include "DynamicBiasWidgets.moc"

