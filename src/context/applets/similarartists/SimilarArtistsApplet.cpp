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

//Amarok
#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "context/Svg.h"
#include "context/ContextView.h"
#include "context/widgets/DropPixmapItem.h"
#include "PaletteHandler.h"

//Kde
#include <Plasma/Theme>
#include <plasma/widgets/iconwidget.h>
#include <KConfigDialog>
#include <KStandardDirs>

//Qt
#include <QDesktopServices>
#include <QGraphicsSimpleTextItem>
#include <QTextEdit>
#include <qgraphicsproxywidget.h>
#include <QLabel>
 #include <QGraphicsGridLayout>
 #include <QGraphicsScene>


SimilarArtistsApplet::SimilarArtistsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_aspectRatio( 0 )
    , m_headerAspectRatio( 0.0 )
    , m_headerLabel( 0 )
    , m_settingsIcon( 0 )    
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );

    m_layout=new QGraphicsGridLayout;
    m_scene=new QGraphicsScene;
    
    m_artistImage = new QLabel;
    m_artistImage->setAttribute( Qt::WA_TranslucentBackground, true); // The background of the QLabel is transparent
    m_artistImage->setScaledContents(true);                           // The QLabel scale is content
    
    m_artistName = new QLabel;
    m_artistName->setAttribute( Qt::WA_TranslucentBackground, true); // The background of the QLabel is transparent
    
    m_artistGenre =new QLabel;
    m_artistGenre->setAttribute( Qt::WA_TranslucentBackground, true); // The background of the QLabel is transparent

    
    QGraphicsProxyWidget *img = m_scene->addWidget(m_artistImage);
    QGraphicsProxyWidget *art = m_scene->addWidget(m_artistName);
    QGraphicsProxyWidget *genre = m_scene->addWidget(m_artistGenre);
    
    m_layout->setRowPreferredHeight(1,80);
    m_layout->setRowPreferredHeight(2,80);
    m_layout->setRowMaximumHeight(1,80);
    m_layout->setRowMaximumHeight(2,80);
    
    m_layout->addItem(img,1,0,2,1);
    m_layout->addItem(art,1,1);
    m_layout->addItem(genre,2,1);
    
    setLayout(m_layout);    
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
    if( source == "similarArtists" ) {
        dataEngine( "amarok-similarArtists" )->connectSource( "similarArtists", this );
        dataUpdated( source, dataEngine( "amarok-similarArtists" )->query( "similarArtists" ) );
    }
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

    // the layout begin at the bottom of the applet's title
    m_layout->setRowMinimumHeight(0,m_headerLabel->boundingRect().height());
    m_layout->setRowMaximumHeight(0,m_headerLabel->boundingRect().height());

    
    QString artistName = data[ "artist" ].toString();

    // we see if the artist name is valid
    if (artistName.compare( "" ) != 0) {
        m_headerLabel->setText( i18n( "Similar artists of " ) + artistName );
        m_artistName->setText("<a href='http://amarok.kde.org'>" + artistName +"</a>");
    } else { // the artist name is invalid
        m_headerLabel->setText( i18n( "Similar Artists" ) );
        m_artistName->setText("Non dispo");
    }

    m_artistImage->setPixmap( data[ "cover" ].value<QPixmap>() );
    m_artistGenre->setText(i18n( "Genre") + " : Pop, Jazz");
    
    updateConstraints();
    update();

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

    connect( ui_Settings.spinBox, SIGNAL( valueChanged( int ) ), this, SLOT( changeMaxArtists( int ) ) );
    connect( parent, SIGNAL( okClicked( ) ), this, SLOT( saveSettings( ) ) );
}

void
SimilarArtistsApplet::changeMaxArtists( int value )
{
DEBUG_BLOCK

    m_temp_maxArtists = value;
}

void
SimilarArtistsApplet::saveMaxArtists()
{
DEBUG_BLOCK

    m_maxArtists = m_temp_maxArtists;

    dataEngine( "amarok-similarArtists" )->query( QString( "similarArtists:maxArtists:" ) + m_maxArtists );

    KConfigGroup config = Amarok::config("SimilarArtists Applet");
    config.writeEntry( "maxArtists", m_maxArtists );
    dataEngine( "amarok-similarArtists" )->query( QString( "similarArtists:maxArtists:" ) + m_maxArtists );
}

void
SimilarArtistsApplet::saveSettings()
{
    saveMaxArtists();
}

#include "SimilarArtistsApplet.moc"



