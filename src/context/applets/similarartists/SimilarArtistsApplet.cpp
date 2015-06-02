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

#include "App.h"
#include "EngineController.h"
#include "PaletteHandler.h"
#include "context/Svg.h"
#include "context/ContextView.h"
#include "context/widgets/AppletHeader.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/meta/Meta.h"

#include <KConfigDialog>
#include <KStandardDirs>
#include <KTextBrowser>
#include <Plasma/TextBrowser>
#include <Plasma/Theme>
#include <plasma/widgets/iconwidget.h>

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
        , m_settingsIcon( 0 )
{
    setHasConfigurationInterface( true );
}

SimilarArtistsApplet::~SimilarArtistsApplet()
{
}

void
SimilarArtistsApplet::init()
{
    DEBUG_BLOCK

    // Call the base implementation.
    Context::Applet::init();

    enableHeader( true );
    setHeaderText( i18n( "Similar Artists" ) );

    QAction* backwardAction = new QAction( this );
    backwardAction->setIcon( QIcon::fromTheme( "go-previous" ) );
    backwardAction->setEnabled( false );
    backwardAction->setText( i18n( "Back" ) );
    m_backwardIcon = addLeftHeaderAction( backwardAction );
    connect( m_backwardIcon, SIGNAL(clicked()), this, SLOT(goBackward()) );

    QAction* forwardAction = new QAction( this );
    forwardAction->setIcon( QIcon::fromTheme( "go-next" ) );
    forwardAction->setEnabled( false );
    forwardAction->setText( i18n( "Forward" ) );
    m_forwardIcon = addLeftHeaderAction( forwardAction );
    connect( m_forwardIcon, SIGNAL(clicked()), this, SLOT(goForward()) );

    QAction *currentAction = new QAction( this );
    currentAction->setIcon( QIcon::fromTheme( "filename-artist-amarok" ) );
    currentAction->setEnabled( true );
    currentAction->setText( i18n( "Show Similar Artists for Currently Playing Track" ) );
    m_currentArtistIcon = addRightHeaderAction( currentAction );
    connect( m_currentArtistIcon, SIGNAL(clicked()), this, SLOT(queryForCurrentTrack()) );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( QIcon::fromTheme( "preferences-system" ) );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    m_settingsIcon = addRightHeaderAction( settingsAction );
    connect( m_settingsIcon, SIGNAL(clicked()), this, SLOT(configure()) );

    setCollapseOffHeight( -1 );
    setCollapseHeight( m_header->height() );
    setMinimumHeight( collapseHeight() );
    setPreferredHeight( collapseHeight() );

    // create a scrollarea
    m_scroll = new ArtistsListWidget( this );
    m_scroll->hide();
    connect( m_scroll, SIGNAL(showSimilarArtists(QString)), SLOT(showSimilarArtists(QString)) );
    connect( m_scroll, SIGNAL(showBio(QString)), SLOT(showArtistBio(QString)) );

    m_layout = new QGraphicsLinearLayout( Qt::Vertical, this );
    m_layout->addItem( m_header );
    m_layout->addItem( m_scroll );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config( "SimilarArtists Applet" );
    m_maxArtists = config.readEntry( "maxArtists", "5" ).toInt();

    Plasma::DataEngine *engine = dataEngine( "amarok-similarArtists" );
    connect( engine, SIGNAL(sourceAdded(QString)), SLOT(connectSource(QString)) );
    engine->setProperty( "maximumArtists", m_maxArtists );
    engine->query( "similarArtists" );
}

void
SimilarArtistsApplet::connectSource( const QString &source )
{
    if( source == QLatin1String("similarArtists") )
        dataEngine( "amarok-similarArtists" )->connectSource( source, this );
}

void
SimilarArtistsApplet::dataUpdated( const QString &source, const Plasma::DataEngine::Data &data )
{
    DEBUG_BLOCK
    QString artist = data[ "artist" ].toString();
    if( source == "similarArtists" )
    {
        setBusy( false );
        if( !artist.isEmpty() )
        {
            m_artist = artist;
            SimilarArtist::List list = data[ "similar" ].value<SimilarArtist::List>();
            if( m_similars != list )
            {
                m_similars = list;
                updateNavigationIcons();
                artistsUpdate();
            }
        }
        else
        {
            setHeaderText( i18n( "Similar Artists" ) );
            m_scroll->clear();
            m_scroll->hide();
            setCollapseOn();
        }
    }
}

void
SimilarArtistsApplet::configure()
{
    showConfigurationInterface();
}

void
SimilarArtistsApplet::createConfigurationInterface( KConfigDialog *parent )
{
    parent->setButtons( KDialog::Ok | KDialog::Cancel );

    KConfigGroup config = Amarok::config( "SimilarArtists Applet" );
    QWidget *settings = new QWidget();
    ui_Settings.setupUi( settings );

    ui_Settings.spinBox->setValue( m_maxArtists );

    parent->addPage( settings, i18n( "Similar Artists Settings" ), "preferences-system" );

    connect( parent, SIGNAL(okClicked()), SLOT(saveSettings()) );
}

void
SimilarArtistsApplet::saveSettings()
{
    DEBUG_BLOCK
    m_maxArtists = ui_Settings.spinBox->value();
    Amarok::config( "SimilarArtists Applet" ).writeEntry( "maxArtists", m_maxArtists );
    dataEngine( "amarok-similarArtists" )->setProperty( "maximumArtists", m_maxArtists );
    dataEngine( "amarok-similarArtists" )->query( "similarArtists:forceUpdate" );
}

void
SimilarArtistsApplet::artistsUpdate()
{
    DEBUG_BLOCK
    if( !m_scroll->isEmpty() )
        m_scroll->clear();

    if( !m_similars.isEmpty() )
    {
        setHeaderText( i18n( "Similar Artists to %1", m_artist ) );
        m_scroll->addArtists( m_similars );
        m_scroll->show();
        setCollapseOff();
    }
    else // No similar artist found
    {
        setHeaderText( i18n( "Similar Artists: Not Found" ) );
        m_scroll->hide();
        setCollapseOn();
    }
}

void
SimilarArtistsApplet::showSimilarArtists( const QString &name )
{
    if( m_artist != name )
        m_historyBack.push( m_artist );
    m_historyForward.clear();
    queryArtist( name );
    updateNavigationIcons();
    setBusy( true );
}

void
SimilarArtistsApplet::showArtistBio( const QString &name )
{
    const ArtistWidget *widget = m_scroll->widget( name );
    if( !widget || widget->fullBio().isEmpty() )
        return;

    Plasma::TextBrowser *tb = new Plasma::TextBrowser( 0 );
    tb->nativeWidget()->setFrameShape( QFrame::StyledPanel );
    tb->nativeWidget()->setOpenExternalLinks( true );
    tb->nativeWidget()->setAutoFormatting( QTextEdit::AutoAll );
    tb->nativeWidget()->viewport()->setAutoFillBackground( true );
    tb->nativeWidget()->viewport()->setBackgroundRole( QPalette::AlternateBase );

    QPalette p = tb->palette();
    p.setColor( QPalette::Text, qApp->palette().text().color() );
    tb->setPalette( p );

    QString bio = widget->fullBio();
    KDateTime pub = widget->bioPublished();
    if( pub.isValid() )
    {
        QString pubDate = i18nc( "@item:intext Artist biography published date",
                                 "Published: %1", pub.toString( KDateTime::LocalDate ) );
        bio = QString( "%1<hr>%2" ).arg( pubDate, bio );
    }
    tb->nativeWidget()->setHtml( bio );

    QGraphicsLinearLayout *l = new QGraphicsLinearLayout( Qt::Vertical );
    l->setContentsMargins( 1, 1, 1, 1 );
    l->addItem( tb );
    qreal width = m_scroll->boundingRect().width() * 3 / 5;
    qreal height = m_scroll->boundingRect().height() * 3 / 5;
    QRectF rect( 0, 0, width, height );
    rect.moveCenter( m_scroll->boundingRect().center() );
    QGraphicsWidget *w = new QGraphicsWidget( 0, Qt::Window );
    w->setGeometry( rect );
    w->setLayout( l );
    scene()->addItem( w );
}

void
SimilarArtistsApplet::queryArtist( const QString &name )
{
    dataEngine( "amarok-similarArtists" )->setProperty( "artist", name );
    dataEngine( "amarok-similarArtists" )->query( "similarArtists:artist" );
}

void
SimilarArtistsApplet::queryForCurrentTrack()
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
        return;
    if( Meta::ArtistPtr artist = track->artist() )
        queryArtist( artist->name() );
}

void
SimilarArtistsApplet::goBackward()
{
    if( !m_historyBack.isEmpty() )
    {
        m_historyForward.push( m_artist );
        m_artist = m_historyBack.pop();
        queryArtist( m_artist );
        updateNavigationIcons();
    }
}

void
SimilarArtistsApplet::goForward()
{
    if( !m_historyForward.isEmpty() )
    {
        m_historyBack.push( m_artist );
        m_artist = m_historyForward.pop();
        queryArtist( m_artist );
        updateNavigationIcons();
    }
}

void
SimilarArtistsApplet::updateNavigationIcons()
{
    m_forwardIcon->action()->setEnabled( !m_historyForward.isEmpty() );
    m_backwardIcon->action()->setEnabled( !m_historyBack.isEmpty() );
}

