/****************************************************************************************
 * Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
 * Copyright (c) 2010 Alexandre Mendes <alex.mendes1988@gmail.com>                      *
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

#define DEBUG_PREFIX "SimilarArtistsApplet"

#include "SimilarArtistsApplet.h"

//Amarok
#include "core/support/Amarok.h"
#include "App.h"
#include "EngineController.h"
#include "core/support/Debug.h"
#include "context/Svg.h"
#include "context/ContextView.h"
#include "PaletteHandler.h"
#include "context/widgets/TextScrollingWidget.h"

//Kde
#include <Plasma/Theme>
#include <plasma/widgets/iconwidget.h>
#include <KConfigDialog>
#include <KStandardDirs>

//Qt
#include <QDesktopServices>
#include <QTextEdit>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QScrollBar>

SimilarArtistsApplet::SimilarArtistsApplet( QObject *parent, const QVariantList& args )
        : Context::Applet( parent, args )
        , m_scroll( 0 )
        , m_headerLabel( 0 )
        , m_settingsIcon( 0 )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );

    EngineController *engine = The::engineController();

    connect( engine, SIGNAL( stopped( qint64, qint64 ) ),
             this, SLOT( stopped() ) );
}

SimilarArtistsApplet::~SimilarArtistsApplet()
{
}

void
SimilarArtistsApplet::init()
{
    // Call the base implementation.
    Context::Applet::init();

    // ask for all the CV height
    resize( 500, -1 );

    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerLabel = new TextScrollingWidget( this );
    m_headerLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerLabel->setFont( labelFont );
    m_headerLabel->setText( i18n( "Similar Artists" ) );
    m_headerLabel->setDrawBackground( true );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setEnabled( true );
    m_settingsIcon = addAction( settingsAction );
    m_settingsIcon->setToolTip( i18n( "Settings" ) );
    connect( m_settingsIcon, SIGNAL(clicked()), this, SLOT(configure()) );

    QGraphicsLinearLayout *headerLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    headerLayout->addItem( m_headerLabel );
    headerLayout->addItem( m_settingsIcon );
    headerLayout->setContentsMargins( 0, 4, 0, 2 );

    // create a scrollarea
    m_scroll = new ArtistsListWidget( this );
    m_scroll->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical, this );
    layout->addItem( headerLayout );
    layout->addItem( m_scroll );

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config( "SimilarArtists Applet" );
    m_maxArtists = config.readEntry( "maxArtists", "5" ).toInt();

    Plasma::DataEngine *engine = dataEngine( "amarok-similarArtists" );
    connect( engine, SIGNAL(sourceAdded(QString)), SLOT(connectSource(QString)) );
    engine->query( "similarArtists" );

    updateConstraints();
    update();
}

void
SimilarArtistsApplet::connectSource( const QString &source )
{
    QStringList allowed;
    allowed << "similarArtists" << "description" << "toptrack";

    if( allowed.contains( source ) )
        dataEngine( "amarok-similarArtists" )->connectSource( source, this );
}

void
SimilarArtistsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );

    prepareGeometryChange();
    QString header = m_headerLabel->isEmpty() ? i18n( "Similar Artists" ) : m_headerLabel->text();
    m_headerLabel->setScrollingText( header );
}

void
SimilarArtistsApplet::stopped()
{
    m_scroll->clear();

    m_headerLabel->setScrollingText( i18n( "Similar Artist: No track playing" ) );
}

void
SimilarArtistsApplet::dataUpdated( const QString &source, const Plasma::DataEngine::Data &data )
{
    QString artist = data[ "artist" ].toString();
    if( source == "similarArtists" )
    {
        if( !artist.isEmpty() )
        {
            m_artist = artist;
            m_similars = data[ "similar" ].value<SimilarArtist::List>();
            artistsUpdate();
        }
        else
        {
            m_headerLabel->setScrollingText( i18n( "Similar Artists" ) );
        }
    }
    else if( source == "description" )
    {
        m_scroll->setDescription( artist, data["text"].toString() );
    }
    else if( source == "toptrack" )
    {
        m_scroll->setTopTrack( artist, data["track"].toString() );
    }
}

void
SimilarArtistsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option,
                                      const QRect &contentsRect )
{
    Q_UNUSED( option )
    Q_UNUSED( contentsRect )
    p->setRenderHint( QPainter::Antialiasing );
    addGradientToAppletBackground( p );
}

void
SimilarArtistsApplet::configure()
{
    showConfigurationInterface();
}

void
SimilarArtistsApplet::switchToLang(const QString &lang)
{
    DEBUG_BLOCK
    if (lang == i18n("English") )
        m_descriptionPreferredLang = "en";
    else if (lang == i18n("French") )
        m_descriptionPreferredLang = "fr";
    else if (lang == i18n("German") )
        m_descriptionPreferredLang = "de";
    else if (lang == i18n("Italian") )
        m_descriptionPreferredLang = "it";
    else if (lang == i18n("Spanish") )
        m_descriptionPreferredLang = "es";
    else
        m_descriptionPreferredLang = "aut";

    KConfigGroup config = Amarok::config("SimilarArtists Applet");
    config.writeEntry( "PreferredLang", m_descriptionPreferredLang );
    dataEngine( "amarok-similarArtists" )->query( QString( "similarArtists:lang:" ) + m_descriptionPreferredLang );
}

void
SimilarArtistsApplet::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup config = Amarok::config( "SimilarArtists Applet" );
    QWidget *settings = new QWidget();
    ui_Settings.setupUi( settings );

    if ( m_descriptionPreferredLang == "aut" )
        ui_Settings.comboBox->setCurrentIndex( 0 );
    else if ( m_descriptionPreferredLang == "en" )
        ui_Settings.comboBox->setCurrentIndex( 1 );
    else if ( m_descriptionPreferredLang == "fr" )
        ui_Settings.comboBox->setCurrentIndex( 2 );
    else if ( m_descriptionPreferredLang == "de" )
        ui_Settings.comboBox->setCurrentIndex( 3 );
    else if ( m_descriptionPreferredLang == "it" )
        ui_Settings.comboBox->setCurrentIndex( 4 );
    else if ( m_descriptionPreferredLang == "es" )
        ui_Settings.comboBox->setCurrentIndex( 5 );

    ui_Settings.spinBox->setValue( m_maxArtists );

    parent->addPage( settings, i18n( "Similar Artists Settings" ), "preferences-system" );

    connect( ui_Settings.comboBox, SIGNAL(currentIndexChanged(QString)), SLOT(switchToLang(QString)) );
    connect( parent, SIGNAL(okClicked()), SLOT(saveSettings()) );
}

void
SimilarArtistsApplet::saveSettings()
{
    DEBUG_BLOCK
    m_maxArtists = ui_Settings.spinBox->value();
    Amarok::config( "SimilarArtists Applet" ).writeEntry( "maxArtists", m_maxArtists );
    dataEngine( "amarok-similarArtists" )->query( QString( "similarArtists:maxArtists:%1" ).arg( m_maxArtists ) );
}

void
SimilarArtistsApplet::artistsUpdate()
{
    m_scroll->clear();
    if( !m_similars.isEmpty() )
    {
        m_headerLabel->setScrollingText( i18n( "Similar Artists of %1", m_artist ) );
        m_scroll->addArtists( m_similars );
    }
    else // No similar artist found
    {
        m_headerLabel->setScrollingText( i18n( "Similar Artists: Not Found" ) );
    }
}

#include "SimilarArtistsApplet.moc"
