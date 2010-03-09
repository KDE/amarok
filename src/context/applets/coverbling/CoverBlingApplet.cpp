/****************************************************************************************
 * Copyright (c) 2010 Emmanuel Wagner <manu.wagner@sfr.fr>                         *
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

#include "CoverBlingApplet.h"

// Amarok
#include "Amarok.h"
#include "Debug.h"
#include "context/ContextView.h"
#include "context/widgets/TextScrollingWidget.h"
#include "collection/Collection.h"
#include "collection/CollectionManager.h"
#include "context/widgets/RatingWidget.h"
#include "playlist/PlaylistModelStack.h"
// KDE
#include <KAction>
#include <KColorScheme>
#include <KConfigDialog>
#include <KGlobalSettings>
#include <Plasma/BusyWidget>
#include <Plasma/IconWidget>
#include <Plasma/Theme>

// Qt
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsWidget>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <KStandardDirs>
#include <QDesktopWidget>

#define DEBUG_PREFIX "CoverBlingApplet"

MyGraphicItem::MyGraphicItem( const QPixmap & pixmap, QGraphicsItem * parent, QGraphicsScene * scene )
        : QGraphicsPixmapItem( pixmap, parent, scene )
{
}
void MyGraphicItem::mousePressEvent( QGraphicsSceneMouseEvent * event )
{
    Q_UNUSED( event );
    emit clicked();
}
CoverBlingApplet::CoverBlingApplet( QObject* parent, const QVariantList& args )
        : Context::Applet( parent, args )
{
    DEBUG_BLOCK
    setHasConfigurationInterface( true );
}

void
CoverBlingApplet::init()
{
    setBackgroundHints( Plasma::Applet::NoBackground );
    resize( -1, 400 );
    m_fullsize = false;
    m_layout = new QGraphicsProxyWidget( this );
    m_pictureflow = new PhotoBrowser();
    m_layout->setWidget( m_pictureflow );
    m_pictureflow->setRenderHints( QPainter::HighQualityAntialiasing | QPainter::SmoothPixmapTransform );

    KConfigGroup config = Amarok::config( "CoverBling Applet" );
    m_coversize = config.readEntry( "CoverSize", 200 );
    int reflectioneffect = config.readEntry( "ReflectionEffect", 1 );
    if ( reflectioneffect == 0 )
        m_reflectionEffect = PictureFlow::NoReflection;
    else if ( reflectioneffect == 1 )
        m_reflectionEffect = PictureFlow::PlainReflection;
    else if ( reflectioneffect == 2 )
        m_reflectionEffect = PictureFlow::BlurredReflection;

    m_autojump = config.readEntry( "AutoJump", false );
    m_animatejump = config.readEntry( "AnimateJump", true );

    m_pictureflow->show();

    Amarok::Collection *coll = CollectionManager::instance()->primaryCollection();
    QueryMaker *qm = coll->queryMaker();
    qm->setAutoDelete( true );
    qm->setQueryType( QueryMaker::Album );
    qm->orderBy( Meta::valArtist );

    connect( qm, SIGNAL( newResultReady( QString, Meta::AlbumList ) ),
             this, SLOT( slotAlbumQueryResult( QString, Meta::AlbumList ) ) );
    //connect( qm, SIGNAL(queryDone()), this, SLOT( slideChanged(0)));
    connect( m_pictureflow, SIGNAL( centerIndexChanged( int ) ), this, SLOT( slideChanged( int ) ) );
    connect( m_pictureflow, SIGNAL( doubleClicked( int ) ), this, SLOT( playAlbum( int ) ) );
    qm->run();
    m_label = new QGraphicsSimpleTextItem( this );
    QBrush brush = KColorScheme( QPalette::Active ).foreground( KColorScheme::NormalText );
    m_label ->setBrush( brush );
    QFont labelFont;
    QFont bigFont( labelFont );
    bigFont.setPointSize( bigFont.pointSize() +  2 );
    m_label->setFont( labelFont );

    m_ratingWidget = new RatingWidget( this );
    m_ratingWidget->setRating( 0 );
    m_ratingWidget->setEnabled( FALSE );

    m_blingtofirst = new MyGraphicItem( QPixmap( KStandardDirs::locate( "data", "amarok/images/blingtofirst.png" ) ), this );
    m_blingtolast = new MyGraphicItem( QPixmap( KStandardDirs::locate( "data", "amarok/images/blingtolast.png" ) ), this );
    m_blingfastback = new MyGraphicItem( QPixmap( KStandardDirs::locate( "data", "amarok/images/blingfastback.png" ) ), this );
    m_blingfastforward = new MyGraphicItem( QPixmap( KStandardDirs::locate( "data", "amarok/images/blingfastforward.png" ) ), this );
    m_fullscreen = new MyGraphicItem( QPixmap( KStandardDirs::locate( "data", "amarok/images/blingfullscreen.png" ) ), this );
    m_jumptoplaying = new MyGraphicItem( QPixmap( KStandardDirs::locate( "data", "amarok/images/blingjumptoplaying.png" ) ), this );

    connect( m_blingtofirst, SIGNAL( clicked() ), this, SLOT( skipToFirst() ) );
    connect( m_blingtolast, SIGNAL( clicked() ), this, SLOT( skipToLast() ) );
    connect( m_blingfastback, SIGNAL( clicked() ), m_pictureflow, SLOT( fastBackward() ) );
    connect( m_blingfastforward, SIGNAL( clicked() ), m_pictureflow, SLOT( fastForward() ) );
    connect( m_fullscreen, SIGNAL( clicked() ), this, SLOT( toggleFullscreen() ) );
    connect( m_jumptoplaying, SIGNAL( clicked() ), this, SLOT( jumpToPlaying() ) );
    constraintsEvent();
}

CoverBlingApplet::~CoverBlingApplet()
{
    delete m_blingtofirst;
    delete m_blingtolast;
    delete m_blingfastback;
    delete m_blingfastforward;
    delete m_fullscreen;
    delete m_jumptoplaying;
    delete m_ratingWidget;
    delete m_label;
    delete m_layout;
}
void CoverBlingApplet::slotAlbumQueryResult( QString collectionId, Meta::AlbumList albums ) //SLOT
{
    DEBUG_BLOCK
    Q_UNUSED( collectionId );
    m_pictureflow->fillAlbums( albums );
}
void CoverBlingApplet::slideChanged( int islideindex )
{
    Meta::AlbumPtr album = m_pictureflow->album( islideindex );
    if ( album )
    {
        Meta::ArtistPtr artist = album->albumArtist();
        QString label = album->prettyName();
        if ( artist ) label += " - " + artist->prettyName();
        m_label->setText( label );
        m_label->show();
        int nbtracks = 0;
        int rating = 0;
        foreach( Meta::TrackPtr track, album->tracks() )
        {
            nbtracks++;
            if ( track )
                rating += track->rating();
        }
        if ( nbtracks )
            rating = rating / nbtracks;
        m_ratingWidget->setRating( rating );
    }
}
void CoverBlingApplet::playAlbum( int islideindex )
{
    Meta::AlbumPtr album = m_pictureflow->album( islideindex );
    if ( album )
    {
        The::playlistController()->insertOptioned( album->tracks(), Playlist::LoadAndPlay );
    }

}
void CoverBlingApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )
    prepareGeometryChange();
    int vertical_size = boundingRect().height();
    int horizontal_size = boundingRect().width();
    QSize slideSize( vertical_size / 2, vertical_size / 2 );
    if ( !m_fullsize )
    {
        slideSize.setWidth( m_coversize );
        slideSize.setHeight( m_coversize );
    }
    m_pictureflow->setSlideSize( slideSize );
    m_pictureflow->setReflectionEffect( m_reflectionEffect );
    m_pictureflow->setAnimationTime( 20 );
    m_ratingWidget->setSpacing( 2 );
    m_ratingWidget->setPos( horizontal_size / 2 - 40, vertical_size - 30 );
    m_label ->setPos( horizontal_size / 2 - 40, vertical_size - 50 );
    m_blingtofirst->setOffset( 20, vertical_size - 30 );
    m_blingtolast->setOffset( horizontal_size - 30, vertical_size - 30 );
    m_blingfastback->setOffset( 50, vertical_size - 30 );
    m_blingfastforward->setOffset( horizontal_size - 60, vertical_size - 30 );
    m_fullscreen->setOffset( horizontal_size - 30, 30 );
    m_jumptoplaying->setOffset( horizontal_size - 60, 30 );
    m_pictureflow->resize( horizontal_size, vertical_size );
}
void
CoverBlingApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( p );
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );
    p->setRenderHint( QPainter::Antialiasing );
    // tint the whole applet
    addGradientToAppletBackground( p );
}
void CoverBlingApplet::toggleFullscreen()
{
    if ( m_fullsize )
    {
        resize( -1, 400 );
    }
    else
    {
        //QDesktopWidget* desktop = QApplication::desktop();
        //if (desktop)
        {
            //    QRect rect = desktop->screenGeometry();
            //    resize(rect.width(),rect.height());
            resize( -1, -1 );
        }
    }
    updateConstraints();
    //constraintsEvent();
    m_fullsize = !m_fullsize;
}
void CoverBlingApplet::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );

    if ( m_reflectionEffect == PictureFlow::NoReflection )
        ui_Settings.reflectionEffectCombo->setCurrentIndex( 0 );
    else if ( m_reflectionEffect == PictureFlow::PlainReflection )
        ui_Settings.reflectionEffectCombo->setCurrentIndex( 1 );
    else if ( m_reflectionEffect == PictureFlow::BlurredReflection )
        ui_Settings.reflectionEffectCombo->setCurrentIndex( 2 );
    if ( m_coversize )
        ui_Settings.coversizeSpin->setValue( m_coversize );
    ui_Settings.autoJumpChk->setChecked( m_autojump );
    ui_Settings.animJumpChk->setChecked( m_animatejump );
    parent->addPage( settings, i18n( "Coverbling Settings" ), "preferences-system" );
    connect( parent, SIGNAL( accepted() ), this, SLOT( saveSettings( ) ) );
}
void CoverBlingApplet::saveSettings()
{
    m_coversize = ui_Settings.coversizeSpin->value();
    if ( ui_Settings.reflectionEffectCombo->currentIndex() == 0 )
        m_reflectionEffect = PictureFlow::NoReflection;
    else if ( ui_Settings.reflectionEffectCombo->currentIndex() == 1 )
        m_reflectionEffect = PictureFlow::PlainReflection;
    else if ( ui_Settings.reflectionEffectCombo->currentIndex() == 2 )
        m_reflectionEffect = PictureFlow::BlurredReflection;
    m_autojump = ui_Settings.autoJumpChk->isChecked();
    m_animatejump = ui_Settings.animJumpChk->isChecked();
    KConfigGroup config = Amarok::config( "CoverBling Applet" );
    config.writeEntry( "CoverSize", m_coversize );
    config.writeEntry( "ReflectionEffect", ( int ) m_reflectionEffect );
    config.writeEntry( "AutoJump", m_autojump );
    config.writeEntry( "AnimateJump", m_animatejump );
    constraintsEvent();
}
void CoverBlingApplet::jumpToPlaying()
{
    Meta::TrackPtr track = The::engineController()->currentTrack();

    if ( !track )
        return;
    Meta::AlbumPtr album = track->album();
    if ( !album )
        return;
    int center = m_pictureflow->centerIndex();
    if ( m_pictureflow->album( center ) == album )
        return;
    int nbslides = m_pictureflow->slideCount();
    bool found = false;
    int index = 0;
    for ( int i = 0; i < nbslides;i++ )
    {
        Meta::AlbumPtr current_album = m_pictureflow->album( i );
        if ( current_album == album )
        {
            index = i;
            found = true;
            break;
        }
    }
    if ( found )
    {
        if ( m_animatejump )
        {
            if ( center - index > 10 || index - center > 10 )
            {
                if ( index > center )
                    m_pictureflow->skipToSlide( index - 10 );
                else
                    m_pictureflow->skipToSlide( index + 10 );
            }
            m_pictureflow->showSlide( index );
        }
        else
            m_pictureflow->skipToSlide( index );
    }
    slideChanged( index );
}
void CoverBlingApplet::engineNewTrackPlaying( )
{
    if ( m_autojump )
    {
        jumpToPlaying();
    }
}
void CoverBlingApplet::skipToFirst()
{
    m_pictureflow->skipToSlide( 0 );
    slideChanged( 0 );
}
void CoverBlingApplet::skipToLast()
{
    int nbslides = m_pictureflow->slideCount();
    m_pictureflow->skipToSlide( nbslides - 1 );
    slideChanged( nbslides - 1 );
}

#include "CoverBlingApplet.moc"

