/***************************************************************************
                     playerwidget.cpp  -  description
                        -------------------
begin                : Mit Nov 20 2002
copyright            : (C) 2002 by Mark Kretschmann
email                :
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarokbutton.h"
#include "browserwin.h"
#include "effectwidget.h"
#include "playerapp.h"
#include "playerwidget.h"
#include "playlistwidget.h"
#include "analyzers/baranalyzer.h"
#include "analyzers/baranalyzer2.h"
#include "analyzers/distortanalyzer.h"
#include "analyzers/turbine.h"
#include "analyzers/spectralshine.h"
#include "amarokdcophandler.h"

#include <qbitmap.h>
#include <qclipboard.h>
#include <qevent.h>
#include <qfont.h>
#include <qframe.h>
#include <qiconset.h>
#include <qimage.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qrect.h>
#include <qslider.h>
#include <qstring.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <qwidget.h>
#include <qtimer.h>

#include <kaction.h>
#include <kbugreport.h>
#include <kdebug.h>
#include <kglobalaccel.h>
#include <khelpmenu.h>
#include <kkeydialog.h>
#include <klistview.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>
#include <ksystemtray.h>
#include <kmessagebox.h>

// CLASS AmarokSlider ------------------------------------------------------------

AmarokSlider::AmarokSlider( QWidget *parent ) : QSlider( parent )
{}


AmarokSlider::~AmarokSlider()
{}


void AmarokSlider::mousePressEvent( QMouseEvent *e )
{
    float newVal;

    if ( orientation() == QSlider::Horizontal )
        newVal = static_cast<float>( e->x() ) / static_cast<float>( width() ) * maxValue();
    else
        newVal = static_cast<float>( e->y() ) / static_cast<float>( height() ) * maxValue();

    int intVal = static_cast<int>( newVal );

    if ( ( intVal < value() - 10 ) || ( intVal > value() + 10 ) )
    {
        pApp->m_bSliderIsPressed = true;
        setValue( intVal );
        emit sliderReleased();
    }

    QSlider::mousePressEvent( e );
}


// CLASS AmarokSystray ------------------------------------------------------------

// AmarokSystray
// FIXME Move implementation to separate sourcefile
AmarokSystray::AmarokSystray( PlayerWidget *playerWidget, KActionCollection *ac ) : KSystemTray( playerWidget )
{
    //    setPixmap( KSystemTray::loadIcon("amarok") ); // @since 3.2
    setPixmap( kapp->miniIcon() ); // 3.1 compatibility for 0.7

    // <berkus> Since it doesn't come to you well, i'll explain it here:
    // We put playlist actions last because: 1) you don't want to accidentally
    // switch amaroK off by pushing rmb on tray icon and then suddenly lmb on the
    // bottom item. 2) if you do like in case 1) the most frequent operation is to
    // change to next track, so it must be at bottom. [usability]

    //<mxcl> usability is much more than just making it so you don't have to move the mouse
    //far, in fact that's a tiny segment of the overly broad term, "usability" and is called ergonomics.
    //Another element of usability, and far more important here than ergonomics, is consistency.
    //Every KDE app has the quit button at the bottom. We should do the same.
    //Also having the actions at the bottom for the reasons described only works when the panel is
    //at the bottom of the screen. This can not be guarenteed.
    //Finally the reasons berkus gave are less relevant now since all the actions can be controlled by
    //various mouse actions over the tray icon.

    contextMenu()->clear();
    contextMenu()->insertTitle( kapp->miniIcon(), kapp->caption() );

    ac->action( "options_configure" )->plug( contextMenu() );
    contextMenu()->insertItem( i18n( "&Help" ), (QPopupMenu *)playerWidget->helpMenu() );
    ac->action( "file_quit" )->plug( contextMenu() );

    contextMenu()->insertSeparator();

    contextMenu()->insertItem( QIconSet( locate( "data", "amarok/images/b_prev.png" ) ),
                               i18n( "[&Z] Prev" ), kapp, SLOT( slotPrev() ) );
    contextMenu()->insertItem( QIconSet( locate( "data", "amarok/images/b_play.png" ) ),
                               i18n( "[&X] Play" ), kapp, SLOT( slotPlay() ) );
    contextMenu()->insertItem( QIconSet( locate( "data", "amarok/images/b_pause.png" ) ),
                               i18n( "[&C] Pause" ), kapp, SLOT( slotPause() ) );
    contextMenu()->insertItem( QIconSet( locate( "data", "amarok/images/b_stop.png" ) ),
                               i18n( "[&V] Stop" ), kapp, SLOT( slotStop() ) );
    contextMenu()->insertItem( QIconSet( locate( "data", "amarok/images/b_next.png" ) ),
                               i18n( "[&B] Next" ), kapp, SLOT( slotNext() ) );

    // don't love them just yet
    setAcceptDrops( false );
}


void AmarokSystray::wheelEvent( QWheelEvent *e )
{
    if ( e->orientation() == Horizontal )
        return ;

    switch ( e->state() )
    {
    case ShiftButton:
        static_cast<PlayerApp *>( kapp ) ->m_pPlayerWidget->wheelEvent( e );
        break;
    default:
        if ( e->delta() > 0 )
            static_cast<PlayerApp *>( kapp ) ->slotNext();
        else
            static_cast<PlayerApp *>( kapp ) ->slotPrev();
        break;
    }

    e->accept();
}



void AmarokSystray::mousePressEvent( QMouseEvent *e )
{
    if( e->button() == MidButton )
    {
        if( static_cast<PlayerApp *>(kapp)->isPlaying() )
            static_cast<PlayerApp *>(kapp)->slotPause();
        else
            static_cast<PlayerApp *>(kapp)->slotPlay();
    }
    else
        KSystemTray::mousePressEvent( e );
}


// CLASS PlayerWidget ------------------------------------------------------------

PlayerWidget::PlayerWidget( QWidget *parent, const char *name )
        : QWidget( parent, name )
        , m_pActionCollection( new KActionCollection( this ) )
        , m_pPopupMenu( NULL )
        , m_pVis( NULL )
        , m_pPlayObjConfigWidget( NULL )
        , m_visTimer( new QTimer( this ) )
        , m_helpMenu( new KHelpMenu( this, KGlobal::instance()->aboutData(), m_pActionCollection ) )
        , m_pDcopHandler( new AmarokDcopHandler )
{
    setCaption( "amaroK" );
    setPaletteForegroundColor( pApp->m_fgColor );

    //actions
    KStdAction::aboutApp( pApp, SLOT( slotShowAbout() ), m_pActionCollection );
    KStdAction::helpContents( pApp, SLOT( slotShowHelp() ), m_pActionCollection );
    KStdAction::tipOfDay( pApp, SLOT( slotShowTip() ), m_pActionCollection );
    KStdAction::reportBug( this, SLOT( slotReportBug() ), m_pActionCollection );
    KStdAction::keyBindings( this, SLOT( slotConfigShortcuts() ), m_pActionCollection );
    KStdAction::keyBindings( this, SLOT( slotConfigGlobalShortcuts() ), m_pActionCollection,
                             "options_configure_global_keybinding" )->
    setText( i18n( "Configure Global Shortcuts..." ) );
    KStdAction::preferences( pApp, SLOT( slotShowOptions() ), m_pActionCollection );
    KStdAction::quit( pApp, SLOT( quit() ), m_pActionCollection );
    KStdAction::copy( this, SLOT( slotConfigGlobalShortcuts() ), m_pActionCollection,
                      "copy_clipboard" )->setText( i18n( "Copy Current Title to Clipboard" ) );

    //     new KAction( "Copy Current Title to Clipboard", CTRL + Key_C,
    //                  this, SLOT( slotCopyClipboard() ), m_pActionCollection, "copy_clipboard" );


    // amaroK background pixmap
    m_oldBgPixmap.resize( size() );

    if ( paletteBackgroundPixmap() )
        m_oldBgPixmap = *paletteBackgroundPixmap();
    else
        m_oldBgPixmap.fill( pApp->m_bgColor );

    setPaletteBackgroundPixmap( QPixmap( locate( "data", "amarok/images/amaroKonlyHG_w320.jpg" ) ) );

    m_pFrame = new QFrame( this );

    //layout, widgets, assembly
    m_pFrameButtons = new QFrame( this );
    m_pFrameButtons->setPaletteBackgroundPixmap( m_oldBgPixmap );

    m_pSlider = new AmarokSlider( this );
    m_pSlider->setOrientation( QSlider::Horizontal );
    m_pSlider->setPageStep( 1 );
    m_pSlider->setValue( 0 );
    m_pSlider->setTracking( true );
    m_pSlider->setFocusPolicy( QWidget::NoFocus );

    m_pSliderVol = new AmarokSlider( this );
    m_pSliderVol->setOrientation( QSlider::Vertical );
    m_pSliderVol->setPageStep( 1 );
    m_pSliderVol->setValue( 0 );
    m_pSliderVol->setTracking( true );
    m_pSliderVol->setFocusPolicy( QWidget::NoFocus );

    QString pathStr( locate( "data", "amarok/images/b_prev.png" ) );

    if ( pathStr == QString::null )
        KMessageBox::sorry( this, i18n( "Error: Could not find icons. Did you forget make install?" ),
                            i18n( "amaroK Error" ) );

    //<Player Buttons>
    QIconSet iconSet;

    m_pButtonPrev = new QPushButton( m_pFrameButtons );
    iconSet.setPixmap( locate( "data", "amarok/images/b_prev.png" ),
                       QIconSet::Automatic, QIconSet::Normal, QIconSet::Off );
    iconSet.setPixmap( locate( "data", "amarok/images/b_prev_down.png" ),
                       QIconSet::Automatic, QIconSet::Normal, QIconSet::On );
    m_pButtonPrev->setIconSet( iconSet );
    m_pButtonPrev->setFocusPolicy( QWidget::NoFocus );
    m_pButtonPrev->setFlat( true );

    m_pButtonPlay = new QPushButton( m_pFrameButtons );
    iconSet.setPixmap( locate( "data", "amarok/images/b_play.png" ),
                       QIconSet::Automatic, QIconSet::Normal, QIconSet::Off );
    iconSet.setPixmap( locate( "data", "amarok/images/b_play_down.png" ),
                       QIconSet::Automatic, QIconSet::Normal, QIconSet::On );
    m_pButtonPlay->setIconSet( iconSet );
    m_pButtonPlay->setFocusPolicy( QWidget::NoFocus );
    m_pButtonPlay->setToggleButton( true );
    m_pButtonPlay->setFlat( true );

    m_pButtonPause = new QPushButton( m_pFrameButtons );
    iconSet.setPixmap( locate( "data", "amarok/images/b_pause.png" ),
                       QIconSet::Automatic, QIconSet::Normal, QIconSet::Off );
    iconSet.setPixmap( locate( "data", "amarok/images/b_pause_down.png" ),
                       QIconSet::Automatic, QIconSet::Normal, QIconSet::On );
    m_pButtonPause->setIconSet( iconSet );
    m_pButtonPause->setFocusPolicy( QWidget::NoFocus );
    m_pButtonPause->setFlat( true );

    m_pButtonStop = new QPushButton( m_pFrameButtons );
    iconSet.setPixmap( locate( "data", "amarok/images/b_stop.png" ),
                       QIconSet::Automatic, QIconSet::Normal, QIconSet::Off );
    iconSet.setPixmap( locate( "data", "amarok/images/b_stop_down.png" ),
                       QIconSet::Automatic, QIconSet::Normal, QIconSet::On );
    m_pButtonStop->setIconSet( iconSet );
    m_pButtonStop->setFocusPolicy( QWidget::NoFocus );
    m_pButtonStop->setFlat( true );

    m_pButtonNext = new QPushButton( m_pFrameButtons );
    iconSet.setPixmap( locate( "data", "amarok/images/b_next.png" ),
                       QIconSet::Automatic, QIconSet::Normal, QIconSet::Off );
    iconSet.setPixmap( locate( "data", "amarok/images/b_next_down.png" ),
                       QIconSet::Automatic, QIconSet::Normal, QIconSet::On );
    m_pButtonNext->setIconSet( iconSet );
    m_pButtonNext->setFocusPolicy( QWidget::NoFocus );
    m_pButtonNext->setFlat( true );
    //</Player Buttons>

    QBoxLayout* lay = new QVBoxLayout( this );
    lay->addWidget( m_pFrame );

    QBoxLayout *lay3 = new QHBoxLayout( lay );
    m_pLay6 = new QVBoxLayout( lay3 );
    m_pLay6->addItem( new QSpacerItem( 0, 2 ) );

    QBoxLayout *lay7 = new QVBoxLayout( lay3 );
    QBoxLayout *lay5 = new QHBoxLayout( lay7 );

    m_pButtonLogo = new AmarokButton( this, locate( "data", "amarok/images/logo_new_active.png" ),
                                      locate( "data", "amarok/images/logo_new_inactive.png" ), false );
    lay5->addItem( new QSpacerItem( 4, 0 ) );
    lay5->addWidget( m_pButtonLogo );
    m_pTimeDisplayLabel = new QLabel( this );
    lay7->addItem( new QSpacerItem( 0, 3 ) );
    lay7->addWidget( m_pTimeDisplayLabel );

    QBoxLayout *lay4 = new QVBoxLayout( lay5 );
    m_pButtonPl = new AmarokButton( this, locate( "data", "amarok/images/pl_active.png" ),
                                    locate( "data", "amarok/images/pl_inactive.png" ), true );
    m_pButtonEq = new AmarokButton( this, locate( "data", "amarok/images/eq_active.png" ),
                                    locate( "data", "amarok/images/eq_inactive.png" ), false );
    lay4->addWidget( m_pButtonPl );
    lay4->addItem( new QSpacerItem( 0, 1 ) );
    lay4->addWidget( m_pButtonEq );
    lay4->addItem( new QSpacerItem( 2, 0 ) );

    lay->addItem( new QSpacerItem( 0, 5 ) );
    QBoxLayout* lay2 = new QHBoxLayout( lay );
    lay2->addItem( new QSpacerItem( 1, 0 ) );
    lay2->addWidget( m_pSlider );
    lay2->addItem( new QSpacerItem( 1, 0 ) );

    lay3->addItem( new QSpacerItem( 1, 0 ) );
    lay3->addWidget( m_pSliderVol );
    lay3->addItem( new QSpacerItem( 1, 0 ) );

    lay->addWidget( m_pFrameButtons );
    QBoxLayout *layButtons = new QHBoxLayout( m_pFrameButtons );
    layButtons->addWidget( m_pButtonPrev );
    layButtons->addWidget( m_pButtonPlay );
    layButtons->addWidget( m_pButtonPause );
    layButtons->addWidget( m_pButtonStop );
    layButtons->addWidget( m_pButtonNext );

    lay->setResizeMode( QLayout::FreeResize );
    layButtons->setResizeMode( QLayout::FreeResize );
    lay2->setResizeMode( QLayout::FreeResize );
    lay4->setResizeMode( QLayout::FreeResize );
    lay5->setResizeMode( QLayout::FreeResize );
    lay7->setResizeMode( QLayout::FreeResize );

    // set up system tray
    m_pTray = new AmarokSystray( this, m_pActionCollection );
    m_pTray->show();
    QToolTip::add( m_pTray, i18n( "amaroK media player" ) );

    // some sizing details
    initScroll(); //requires m_pFrame to be created
    setFixedSize( 310, 130 + m_pFrame->height() ); //was 155
    initTimeDisplay();
    m_pTimeDisplayLabel->setFixedSize( 9 * 12 + 2, 12 + 2 );
    timeDisplay( false, 0, 0, 0 );

    // connect vistimer
    connect( m_visTimer, SIGNAL( timeout() ), pApp, SLOT( slotVisTimer() ) );
}


PlayerWidget::~PlayerWidget()
{}


// METHODS ----------------------------------------------------------------

void PlayerWidget::initScroll()
{
    //so, the font selection in the options doesn't work, but since we offer font selection there
    //here we should show the font the user has already chosen, ie the KDE default font.
    //FIXME get the font selection working
    //      I feel this should wait until we implement KConfig XT since that will make life easier

    //QFont font( "Helvetica", 10 );
    //font.setStyleHint( QFont::Helvetica );
    //int frameHeight = QFontMetrics( font ).height() + 5;
    int frameHeight = fontMetrics().height() + 5;

    m_pFrame->setFixedSize( width(), frameHeight );
    //m_pFrame->setFont( font );

    m_pixmapWidth  = 800;
    m_pixmapHeight = frameHeight; //m_optPlayerWidgetScrollFont

    m_pBgPixmap = new QPixmap( paletteBackgroundPixmap() ->convertToImage().copy( m_pFrame->x(),
                               m_pFrame->y(), m_pFrame->width(), m_pFrame->height() ) );

    m_pComposePixmap = new QPixmap( m_pFrame->width(), m_pixmapHeight );
    m_pScrollPixmap = new QPixmap( m_pixmapWidth, m_pixmapHeight );
    m_pScrollMask = new QBitmap( m_pixmapWidth, m_pixmapHeight );
    setScroll( i18n( "   welcome to amaroK   " ), " ", " " );

    m_sx = m_sy = 0;
    m_sxAdd = 1;
}


void PlayerWidget::initTimeDisplay()
{
    m_timeDisplayX = 190;
    m_timeDisplayY = 84;
    m_timeDisplayW = 12;

    m_pTimePixmap = new QPixmap( locate( "data", "amarok/images/numbers_transp.png" ) );

    m_pTimeBgPixmap = new QPixmap( paletteBackgroundPixmap() ->convertToImage()
                                   .copy( m_timeDisplayX, m_timeDisplayY, 9 * m_timeDisplayW, m_timeDisplayW ) );

    m_pTimeComposePixmap = new QPixmap( m_pTimeBgPixmap->width(), m_pTimeBgPixmap->height() );
}


void PlayerWidget::polish()
{
    QWidget::polish();
}


void PlayerWidget::setScroll( QString text, QString bitrate, QString samplerate )
{
    /* Update tray tooltip */
    if ( QToolTip::textFor( m_pTray ) != QString::null ) QToolTip::remove( m_pTray );
    QToolTip::add( m_pTray, text );

    m_pDcopHandler->setNowPlaying( text );

    m_bitrate = bitrate;
    m_samplerate = samplerate;
    text.prepend( "   ***   " );

    m_pScrollMask->fill( Qt::color0 );
    QPainter painterPix( m_pScrollPixmap );
    QPainter painterMask( m_pScrollMask );
    painterPix.setBackgroundColor( Qt::black );
    painterPix.setPen( pApp->m_fgColor );
    painterMask.setPen( Qt::color1 );

    painterPix.setFont( m_pFrame->font() );
    painterMask.setFont( m_pFrame->font() );

    painterPix.eraseRect( 0, 0, m_pixmapWidth, m_pixmapHeight );
    painterPix.drawText( 0, 0, m_pixmapWidth, m_pixmapHeight, Qt::AlignLeft || Qt::AlignVCenter, text );
    painterMask.drawText( 0, 0, m_pixmapWidth, m_pixmapHeight, Qt::AlignLeft || Qt::AlignVCenter, text );
    m_pScrollPixmap->setMask( *m_pScrollMask );

    QRect rect = painterPix.boundingRect( 0, 0, m_pixmapWidth, m_pixmapHeight,
                                          Qt::AlignLeft || Qt::AlignVCenter, text );
    m_scrollWidth = rect.width();

    // trigger paintEvent, so the Bitrate and Samplerate text gets drawn
    update();
}


void PlayerWidget::drawScroll()
{
    bitBlt( m_pComposePixmap, 0, 0, m_pBgPixmap );

    m_sx += m_sxAdd;
    if ( m_sx >= m_scrollWidth )
        m_sx = 0;

    int marginH = 4;
    int marginV = 3;
    int subs = 0;
    int dx = marginH;
    int sxTmp = m_sx;

    while ( dx < m_pFrame->width() )
    {
        subs = -m_pFrame->width() + marginH;
        subs += dx + ( m_scrollWidth - sxTmp );
        if ( subs < 0 )
            subs = 0;
        bitBlt( m_pComposePixmap, dx, marginV,
                m_pScrollPixmap, sxTmp, m_sy, m_scrollWidth - sxTmp - subs, m_pixmapHeight, Qt::CopyROP );
        dx += ( m_scrollWidth - sxTmp );
        sxTmp += ( m_scrollWidth - sxTmp ) ;

        if ( sxTmp >= m_scrollWidth )
            sxTmp = 0;
    }

    bitBlt( m_pFrame, 0, 0, m_pComposePixmap );
}


void PlayerWidget::timeDisplay( bool remaining, int hours, int minutes, int seconds )
{
    bitBlt( m_pTimeComposePixmap, 0, 0, m_pTimeBgPixmap );

    int x = 0;
    int y = 0;

    if ( hours > 60 || hours < 0 )
        hours = 0;
    if ( minutes > 60 || minutes < 0 )
        minutes = 0;
    if ( seconds > 60 || seconds < 0 )
        seconds = 0;

    if ( remaining )
        bitBlt( m_pTimeComposePixmap, x, y, m_pTimePixmap, 11 * m_timeDisplayW, 0, m_timeDisplayW );
    else
        bitBlt( m_pTimeComposePixmap, x, y, m_pTimePixmap, 12 * m_timeDisplayW, 0, m_timeDisplayW );

    x += m_timeDisplayW;

    bitBlt( m_pTimeComposePixmap, x, y, m_pTimePixmap, hours / 10 * m_timeDisplayW, 0, m_timeDisplayW );
    x += m_timeDisplayW;
    bitBlt( m_pTimeComposePixmap, x, y, m_pTimePixmap, hours % 10 * m_timeDisplayW, 0, m_timeDisplayW );
    x += m_timeDisplayW;
    bitBlt( m_pTimeComposePixmap, x, y, m_pTimePixmap, 10 * m_timeDisplayW, 0, m_timeDisplayW );
    x += m_timeDisplayW;
    bitBlt( m_pTimeComposePixmap, x, y, m_pTimePixmap, minutes / 10 * m_timeDisplayW, 0, m_timeDisplayW );
    x += m_timeDisplayW;
    bitBlt( m_pTimeComposePixmap, x, y, m_pTimePixmap, minutes % 10 * m_timeDisplayW, 0, m_timeDisplayW );
    x += m_timeDisplayW;
    bitBlt( m_pTimeComposePixmap, x, y, m_pTimePixmap, 10 * m_timeDisplayW, 0, m_timeDisplayW );
    x += m_timeDisplayW;
    bitBlt( m_pTimeComposePixmap, x, y, m_pTimePixmap, seconds / 10 * m_timeDisplayW, 0, m_timeDisplayW );
    x += m_timeDisplayW;
    bitBlt( m_pTimeComposePixmap, x, y, m_pTimePixmap, seconds % 10 * m_timeDisplayW, 0, m_timeDisplayW );


    m_pTimeDisplayLabel->setPixmap( *m_pTimeComposePixmap );
}


// EVENTS -----------------------------------------------------------------

void PlayerWidget::paintEvent( QPaintEvent * )
{
    erase( 20, 40, 120, 50 );
    QPainter pF( this );

    QFont font( "Helvetica", 8 );
    font.setStyleHint( QFont::Helvetica );
    pF.setFont( font );
    pF.setPen( pApp->m_fgColor );

    /*
        pF.drawText( 20, 40, m_bitrate );
        pF.drawText( 70, 40, m_samplerate );
    */
    //<mxcl> was above, however this wasn't working for me as at 1280x1024 I have fonts with lots of pixels
    //<mxcl> we can use QFontMetrics, however, we should decide on how to present these datas first!
    //<mxcl> below is temporary solution
    pF.drawText( 20, 40, m_bitrate + "  " + m_samplerate );
}


void PlayerWidget::mouseReleaseEvent( QMouseEvent *e )
{
    if ( m_pTimeDisplayLabel->geometry().contains( e->pos() ) )
    {
        pApp->m_optTimeDisplayRemaining = !pApp->m_optTimeDisplayRemaining;
    }
}


void PlayerWidget::wheelEvent( QWheelEvent *e )
{
    e->accept();
    pApp->m_Volume += ( e->delta() * -1 ) / 18;

    if ( pApp->m_Volume < 0 )
        pApp->m_Volume = 0;
    if ( pApp->m_Volume > 100 )
        pApp->m_Volume = 100;

    pApp->slotVolumeChanged( pApp->m_Volume );
    m_pSliderVol->setValue( pApp->m_Volume );
}


void PlayerWidget::mousePressEvent( QMouseEvent *e )
{
    if ( e->button() == QMouseEvent::RightButton )
    {
        if ( !m_pPopupMenu )
        {
            m_pPopupMenu = new QPopupMenu( this );
            m_pPopupMenu->setCheckable( true );

            m_IdRepeatTrack = m_pPopupMenu->insertItem( i18n( "Repeat &Track" ), pApp, SLOT( slotSetRepeatTrack() ) );
            m_IdRepeatPlaylist = m_pPopupMenu->insertItem( i18n( "Repeat Play&list" ), pApp, SLOT( slotSetRepeatPlaylist() ) );
            m_IdRandomMode = m_pPopupMenu->insertItem( i18n( "Random &Mode" ), pApp, SLOT( slotSetRandomMode() ) );

            m_pPopupMenu->insertSeparator();

            m_pPopupMenu->insertItem( i18n( "Configure &Effects..." ), pApp, SLOT( slotConfigEffects() ) );
            m_IdConfPlayObject = m_pPopupMenu->insertItem( i18n( "Configure &PlayObject..." ), this, SLOT( slotConfigPlayObject() ) );

            m_pPopupMenu->insertSeparator();

            m_pActionCollection->action( "options_configure_keybinding" )->plug( m_pPopupMenu );
            m_pActionCollection->action( "options_configure_global_keybinding" )->plug( m_pPopupMenu );
            m_pActionCollection->action( "options_configure" )->plug( m_pPopupMenu );

            m_pPopupMenu->insertSeparator();

            m_pPopupMenu->insertItem( i18n( "&Help" ), (QPopupMenu*)helpMenu() );

            m_pPopupMenu->insertSeparator();

            m_pActionCollection->action( "file_quit" )->plug( m_pPopupMenu );
        }

        m_pPopupMenu->setItemChecked( m_IdRepeatTrack, pApp->m_optRepeatTrack );
        m_pPopupMenu->setItemChecked( m_IdRepeatPlaylist, pApp->m_optRepeatPlaylist );
        m_pPopupMenu->setItemChecked( m_IdRandomMode, pApp->m_optRandomMode );

        m_pPopupMenu->setItemEnabled( m_IdConfPlayObject, pApp->playObjectConfigurable() );

        m_pPopupMenu->exec( e->globalPos() );
    }
}


void PlayerWidget::closeEvent( QCloseEvent *e )
{
    //KDE policy states we should hide to tray and not quit() when the close window button is
    //pushed for the main widget -mxcl
    //of course since we haven't got an obvious quit button, this is not yet a perfect solution..

    if( pApp->m_optShowTrayIcon )
    {
        KMessageBox::information( this,
                                  i18n( "<qt>Closing the main window will keep amaroK running in the system tray. "
                                        "Use Quit from the popup-menu to quit the application.</qt>" ),
                                  i18n( "Docking in System Tray" ), "hideOnCloseInfo" );
        e->accept();
    }
    else if ( pApp->queryClose() )
        pApp->quit();
}


void PlayerWidget::moveEvent( QMoveEvent * )
{
    //     You can get the frame sizes like so (found in Qt sources while looking for something else):
    /*    int framew = geometry().x() - x();
        int frameh = geometry().y() - y();*/

    // Makes the the playlistwindow stick magnetically to the playerwindow

    /*    if ( pApp->m_pBrowserWin->isVisible() )
        {
            if ( ( frameGeometry().x() == pApp->m_pBrowserWin->frameGeometry().right() + 1 ) )
                    ( e->oldPos().y() == pApp->m_pBrowserWin->frameGeometry().bottom() ) ||
                    ( e->oldPos().x() + frameSize().width() + 0 == pApp->m_pBrowserWin->frameGeometry().left() ) ||
                    ( e->oldPos().y() + frameSize().height() + 0 == pApp->m_pBrowserWin->frameGeometry().top() ) )
            {
                pApp->m_pBrowserWin->move( e->pos() + ( pApp->m_pBrowserWin->pos() -  e->oldPos() ) );
                pApp->m_pBrowserWin->move( e->pos() + ( pApp->m_pBrowserWin->pos() -  e->oldPos() ) );
            }
        }*/
}


// SLOTS ---------------------------------------------------------------------

void PlayerWidget::createVis()
{
    delete m_pVis;

    pApp->m_optVisCurrent++;

    switch( pApp->m_optVisCurrent )
    {
    case 0:
    firstcase:
        m_pVis = new BarAnalyzer( this );
        break;
    case 1:
        m_pVis = new DistortAnalyzer( this );
        break;
    case 2:
        m_pVis = new BarAnalyzer2( this );
        break;
    case 3:
        m_pVis = new TurbineAnalyzer( this );
        break;
    case 4:
        m_pVis = new SpectralShineAnalyzer( this );
        break;
    default:
        //oh wise ones! Please forgive my use of the goto command!
        //at first I just called createVis() again, which I felt was quite neat, but then I thought again,
        //is this not a suitable place to use a goto? you're only not meant to use goto commands when a loop
        //would suffice, and here I'm using one instead of pointless function recursion
        //admittedly this is a little bit of a hack anyway.. but it can stay until we have a proper stack for
        //viswidgets IMHO - <mxcl>

        //this is so we don't have to remember how many viswidgets there are
        pApp->m_optVisCurrent = 0;
        goto firstcase;
    }

    m_pVis->setFixedSize( 168, 50 );
    m_pLay6->addWidget( m_pVis );
    connect( m_pVis, SIGNAL( clicked() ), this, SLOT( createVis() ) );

    m_visTimer->start( m_pVis->timeout() );
    m_pVis->show();
}


void PlayerWidget::slotConfigShortcuts()
{
    KKeyDialog keyDialog( true );

    keyDialog.insert( m_pActionCollection, i18n( "Player Window" ) );
    keyDialog.insert( pApp->m_pBrowserWin->m_pActionCollection, i18n( "Playlist Window" ) );

    keyDialog.configure();
}


void PlayerWidget::slotConfigGlobalShortcuts()
{
    KKeyDialog::configure( pApp->m_pGlobalAccel, true, 0, true );
}


void PlayerWidget::slotCopyClipboard()
{
    QListViewItem * currentTrack = pApp->m_pBrowserWin->m_pPlaylistWidget->currentTrack();

    if ( currentTrack )
    {
        QClipboard * cb = QApplication::clipboard();
        cb->setText( currentTrack->text( 0 ) );
    }
}


void PlayerWidget::slotConfigPlayObject()
{
    if ( pApp->m_pPlayObject && !m_pPlayObjConfigWidget )
    {
        m_pPlayObjConfigWidget = new ArtsConfigWidget( pApp->m_pPlayObject->object(), this );

        connect( m_pPlayObjConfigWidget, SIGNAL( destroyed() ), this, SLOT( slotConfigWidgetDestroyed() ) );
        m_pPlayObjConfigWidget->show();
    }
}


void PlayerWidget::slotConfigWidgetDestroyed()
{
    m_pPlayObjConfigWidget = NULL;
}


void PlayerWidget::slotUpdateTrayIcon( bool visible )
{
    if ( visible )
    {
        m_pTray->show();
    }
    else
    {
        m_pTray->hide();
    }
}


void PlayerWidget::slotReportBug()
{
    KBugReport report;
    report.exec();
}


void PlayerWidget::show()
{
    //this is done in show() rather than showEvent() because
    //we need to show() the playlist first or it will steal focus
    emit sigAboutToShow();

    QWidget::show();
}

void PlayerWidget::hide()
{
    emit sigAboutToHide();

    QWidget::hide();
}

#include "playerwidget.moc"
