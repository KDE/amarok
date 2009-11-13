/****************************************************************************************
 * Copyright (c) 2009 Joffrey Clavel <jclavel@clabert.info>                             *
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
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

#include "SimilarArtistsApplet.h"

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "context/Svg.h"
#include "context/ContextView.h"
#include "PaletteHandler.h"

#include <Plasma/Theme>
#include <plasma/widgets/iconwidget.h>

#include <KConfigDialog>
#include <KStandardDirs>


#include <QDesktopServices>
#include <QGraphicsSimpleTextItem>


SimilarArtistsApplet::SimilarArtistsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_aspectRatio( 0 )
    , m_headerAspectRatio( 0.0 )
    , m_headerLabel( 0 )
    , m_settingsIcon( 0 )    
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

void
SimilarArtistsApplet::init()
{
    m_headerLabel = new QGraphicsSimpleTextItem( this );

    // ask for all the CV height
    resize( 500, -1 );

    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerLabel->setFont( labelFont );
    m_headerLabel->setText( i18n( "Similar Artists" ) );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setEnabled( true );
    m_settingsIcon = addAction( settingsAction );
    m_settingsIcon->setToolTip( i18n( "Settings" ) );
    connect( m_settingsIcon, SIGNAL( activated() ), this, SLOT( configure() ) );

    connectSource( "similarArtists" );
    connect( dataEngine( "amarok-similarArtists" ), SIGNAL( sourceAdded( const QString & ) ), SLOT( connectSource( const QString & ) ) );

    constraintsEvent();

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("SimilarArtists Applet");
    m_maxArtists = config.readEntry( "maxArtists", "20" ).toInt();
}

void
SimilarArtistsApplet::connectSource( const QString &source )
{
    if( source == "similarArtists" )
        dataEngine( "amarok-similarArtists" )->connectSource( "similarArtists", this );
}

void
SimilarArtistsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );

    prepareGeometryChange();
    const float textWidth = m_headerLabel->boundingRect().width();
    const float offsetX =  ( boundingRect().width() - textWidth ) / 2;

    m_headerLabel->setPos( offsetX, standardPadding() + 2 );

    // Icon positionning
    m_settingsIcon->setPos( size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );
    
}

bool
SimilarArtistsApplet::hasHeightForWidth() const
{
    return true;
}

qreal
SimilarArtistsApplet::heightForWidth( qreal width ) const
{
    return width * m_aspectRatio;
}

void
SimilarArtistsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    Q_UNUSED( name )
}

void
SimilarArtistsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option )
    Q_UNUSED( contentsRect )

    p->setRenderHint( QPainter::Antialiasing );

    addGradientToAppletBackground( p );

    // draw rounded rect around title
    drawRoundedRectAroundText( p, m_headerLabel );

    //draw background of wiki text
    p->save();
    QColor bg( App::instance()->palette().highlight().color() );
    bg.setHsvF( bg.hueF(), 0.07, 1, bg.alphaF() );

    p->restore();
}


void
SimilarArtistsApplet::configure()
{
    DEBUG_BLOCK
    showConfigurationInterface();

}


void
SimilarArtistsApplet::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup config = Amarok::config("SimilarArtists Applet");
    QWidget *settings = new QWidget();
    ui_Settings.setupUi( settings );

    ui_Settings.spinBox->setValue( m_maxArtists );
    
    parent->addPage( settings, i18n( "Similar Artists Settings" ), "preferences-system");

    connect( ui_Settings.spinBox, SIGNAL( valueChanged( int ) ), this, SLOT( maxArtistsChanged( int ) ) );
}

void
SimilarArtistsApplet::maxArtistsChanged( int value )
{
DEBUG_BLOCK

    m_maxArtists = value;

    dataEngine( "amarok-similarArtists" )->query( QString( "similarArtists:maxArtists:" ) + m_maxArtists );

    KConfigGroup config = Amarok::config("SimilarArtists Applet");
    config.writeEntry( "maxArtists", m_maxArtists );
    dataEngine( "amarok-similarArtists" )->query( QString( "similarArtists:maxArtists:" ) + m_maxArtists );
}

#include "SimilarArtistsApplet.moc"



