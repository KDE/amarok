/***************************************************************************
                     playerwidget.cpp  -  description
                        -------------------
begin                : Mit Nov 20 2002
copyright            : (C) 2002 by Mark Kretschmann
email                : markey@web.de
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
#include "amarokslider.h"
#include "amaroksystray.h"
#include "browserwin.h"
#include "effectwidget.h"
#include "playerapp.h"
#include "playerwidget.h"
#include "playlistwidget.h"
#include "analyzers/blockanalyzer.h"
#include "analyzers/baranalyzer.h"
//#include "analyzers/baranalyzer2.h"
#include "analyzers/distortanalyzer.h"
#include "analyzers/turbine.h"
//#include "analyzers/spectralshine.h"
#include "analyzers/xmasdrug.h"
#include "amarokdcophandler.h"

#include <qbitmap.h>
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
#include <kstandarddirs.h>
#include <ksystemtray.h>
#include <kmessagebox.h>


PlayerWidget::PlayerWidget( QWidget *parent, const char *name )
        : QWidget( parent, name )
        , m_pActionCollection( new KActionCollection( this ) )
        , m_pPopupMenu( NULL )
        , m_pVis( NULL )
        , m_visTimer( new QTimer( this ) )
        , m_helpMenu( new KHelpMenu( this, KGlobal::instance()->aboutData(), m_pActionCollection ) )
        , m_pDcopHandler( new AmarokDcopHandler )
{
    setCaption( "amaroK" );
    setPaletteForegroundColor( pApp->m_fgColor );

    //actions
    KStdAction::keyBindings( this, SLOT( slotConfigShortcuts() ), m_pActionCollection );
    KStdAction::keyBindings( this, SLOT( slotConfigGlobalShortcuts() ), m_pActionCollection,
                             "options_configure_global_keybinding"
                           )->setText( i18n( "Configure Global Shortcuts..." ) );
    KStdAction::preferences( pApp, SLOT( slotShowOptions() ), m_pActionCollection );
    KStdAction::quit( pApp, SLOT( quit() ), m_pActionCollection );


    // amaroK background pixmap
    m_oldBgPixmap.resize( 311, 22 );

    setPaletteBackgroundPixmap( QPixmap( locate( "data", "amarok/images/player_background.jpg" ) ) );

    m_pFrame = new QFrame( this );

    //layout, widgets, assembly
    m_pFrameButtons = new QFrame( this );

    m_pSlider = new AmarokSlider( this, Qt::Horizontal );
    m_pSlider->setFocusPolicy( QWidget::NoFocus );

    m_pSliderVol = new AmarokSlider( this, Qt::Vertical );
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

    m_oldBgPixmap.fill( m_pButtonPlay->paletteBackgroundColor() );

    // MainWindow Layout
    QFont timeFont( "Arial" );
    timeFont.setBold( TRUE );
    timeFont.setPixelSize( 18 );

    m_pTimeDisplayLabel = new QLabel( this );
    m_pTimeDisplayLabel->setFont( timeFont );
    m_pTimeDisplayLabel->setPaletteForegroundColor( QColor( 255, 255, 255 ) );
    m_pTimeDisplayLabel->move( 16, 36 );

    m_pTimePlusPixmap   = new QPixmap( locate( "data", "amarok/images/time_plus.png" ) );
    m_pTimeMinusPixmap  = new QPixmap( locate( "data", "amarok/images/time_minus.png" ) );
    m_pVolSpeaker       = new QPixmap( locate( "data", "amarok/images/vol_speaker.png" ) );
    m_pDescriptionImage = new QPixmap( locate( "data", "amarok/images/description.png" ) );

    m_pDescription = new QLabel (this );
    m_pDescription->move( 4, 5 );
    m_pDescription->setFixedSize( 130, 10 );
    m_pDescription->setPixmap( *m_pDescriptionImage );

    m_pTimeSign = new QLabel( this );
    m_pTimeSign->move( 6, 40 );
    m_pTimeSign->setFixedSize( 10, 10 );

    m_pVolSign = new QLabel( this );
    m_pVolSign->move( 295, 7 );
    m_pVolSign->setFixedSize( 9, 8 );
    m_pVolSign->setPixmap( *m_pVolSpeaker );

/*    m_pButtonLogo = new AmarokButton( this, locate( "data", "amarok/images/logo_new_active.png" ),
                                      locate( "data", "amarok/images/logo_new_inactive.png" ), false );
    m_pButtonLogo->move( -100, -100 );*/

    m_pButtonPl = new AmarokButton( this, locate( "data", "amarok/images/pl_inactive2.png" ),
                                    locate( "data", "amarok/images/pl_active2.png" ), true );
    m_pButtonEq = new AmarokButton( this, locate( "data", "amarok/images/eq_inactive2.png" ),
                                    locate( "data", "amarok/images/eq_active2.png" ), false );

    m_pButtonEq->move( 5, 85 );
    m_pButtonEq->resize( 28, 13 );
    m_pButtonPl->move( 34, 85 );
    m_pButtonPl->resize( 28, 13 );

    m_pSlider->move( 4, 103 );
    m_pSlider->resize( 303, 12 );
    m_pSliderVol->move( 294, 18 );
    m_pSliderVol->resize( 12, 79 );

    m_pFrameButtons->move( 0, 118 );
    m_pFrameButtons->resize( 311, 22 );
    m_pFrameButtons->setPaletteBackgroundPixmap( m_oldBgPixmap );

    m_pButtonPrev->move( 1, 2 );
    m_pButtonPrev->resize( 61, 20 );

    m_pButtonPlay->move( 63, 2 );
    m_pButtonPlay->resize( 61, 20 );

    m_pButtonPause->move( 125, 2 );
    m_pButtonPause->resize( 61, 20 );

    m_pButtonStop->move( 187, 2 );
    m_pButtonStop->resize( 61, 20 );

    m_pButtonNext->move( 249, 2 );
    m_pButtonNext->resize( 61, 20 );

    // set up system tray
    m_pTray = new AmarokSystray( this, m_pActionCollection );
    m_pTray->show();

    // some sizing details
    setFixedSize( 311, 140 ); //y was 130
    initScroll(); //requires m_pFrame to be created

    m_pTimeDisplayLabel->setFixedSize( 9 * 12 + 2, 16 );
    timeDisplay( false, 0, 0, 0 );

    // connect vistimer
    connect( m_visTimer, SIGNAL( timeout() ), pApp, SLOT( slotVisTimer() ) );
//    connect( m_pButtonLogo, SIGNAL( clicked() ), m_helpMenu, SLOT( aboutApplication() ) );
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
    int frameHeight = 16;

    m_pFrame->setFixedSize( 252, frameHeight );
    m_pFrame->move( 3, 14 );
    //m_pFrame->setFont( font );

    m_pixmapWidth  = 2000;
    m_pixmapHeight = frameHeight; //m_optPlayerWidgetScrollFont

    m_pBgPixmap = new QPixmap( paletteBackgroundPixmap() ->convertToImage().copy( m_pFrame->x(),
                               m_pFrame->y(), m_pFrame->width(), m_pFrame->height() ) );

    m_pComposePixmap = new QPixmap( m_pFrame->width(), m_pixmapHeight );
    m_pScrollPixmap = new QPixmap( m_pixmapWidth, m_pixmapHeight );
    m_pScrollMask = new QBitmap( m_pixmapWidth, m_pixmapHeight );
    setScroll();

    m_sx = m_sy = 0;
    m_sxAdd = 1;
}


void PlayerWidget::polish()
{
    QWidget::polish();
}


void PlayerWidget::setScroll( QString text, const QString &bitrate, const QString &sampleRate )
{
    //Update tray tooltip
    if ( QToolTip::textFor( m_pTray ) != QString::null ) QToolTip::remove( m_pTray );
    if ( text.isEmpty() )
    {
        QToolTip::add( m_pTray, i18n( "amaroK - Media Player" ) );
        m_pDcopHandler->setNowPlaying( text ); //text = ""
        m_bitrate = m_samplerate = text; //better to use text than create a temporary QString
                                         //looks better if these are clear
        text = i18n( "Welcome to amaroK" );
    }
    else
    {
        QToolTip::add( m_pTray, text );
        m_pDcopHandler->setNowPlaying( text );
        m_bitrate = bitrate;
        m_samplerate = sampleRate;
    }

    text.prepend( " | " );

    m_pScrollMask->fill( Qt::color0 );
    QPainter painterPix( m_pScrollPixmap );
    QPainter painterMask( m_pScrollMask );
    painterPix.setBackgroundColor( Qt::black );
    painterPix.setPen( QColor( 255, 255, 255 ) );
    painterMask.setPen( Qt::color1 );

    QFont scrollerFont( "Arial" );
    scrollerFont.setBold( TRUE );
    scrollerFont.setPixelSize( 11 );

    painterPix.setFont( scrollerFont );
    painterMask.setFont( scrollerFont );

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
    QString str;

    if ( hours < 10 ) str += "0";
    str += QString::number( hours );
    str += ":";

    if ( minutes < 10 ) str += "0";
    str += QString::number( minutes );
    str += ":";

    if ( seconds < 10 ) str += "0";
    str += QString::number( seconds );

    m_pTimeDisplayLabel->setText( str );

    if (!remaining)
        m_pTimeSign->setPixmap( *m_pTimePlusPixmap );
    else
        m_pTimeSign->setPixmap( *m_pTimeMinusPixmap );
}


// EVENTS -----------------------------------------------------------------

void PlayerWidget::paintEvent( QPaintEvent * )
{
    erase( 20, 40, 120, 50 );
    QPainter pF( this );

    QFont font( "Arial" );
//    font.setStyleHint( QFont::Arial );
    font.setBold( TRUE );
    font.setPixelSize( 10 );
    pF.setFont( font );
//    pF.setPen( pApp->m_fgColor );
    pF.setPen( QColor( 255, 255, 255 ) );

    //this method avoids creation/destruction of temporaries (use +=)
    //also it's visually appealing as it shows nothing if the rates aren't set
    QString str = m_bitrate;
    if( !(m_bitrate.isEmpty() || m_samplerate.isEmpty() ) ) str += " / ";
    str += m_samplerate;
    pF.drawText( 6, 68, str );
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


#define ID_REPEAT_TRACK 100
#define ID_REPEAT_PLAYLIST 101
#define ID_RANDOM_MODE 102
#define ID_CONF_PLAYOBJECT 103

void PlayerWidget::mousePressEvent( QMouseEvent *e )
{
    if ( e->button() == QMouseEvent::RightButton )
    {
        if ( !m_pPopupMenu )
        {
            m_pPopupMenu = new QPopupMenu( this );
            m_pPopupMenu->setCheckable( true );

            m_pPopupMenu->insertItem( i18n( "Repeat &Track" ),    ID_REPEAT_TRACK );
            m_pPopupMenu->insertItem( i18n( "Repeat Play&list" ), ID_REPEAT_PLAYLIST );
            m_pPopupMenu->insertItem( i18n( "Random &Mode" ),     ID_RANDOM_MODE );

            m_pPopupMenu->insertSeparator();

            m_pPopupMenu->insertItem( i18n( "Configure &Effects..." ), pApp, SLOT( slotConfigEffects() ) );
            m_pPopupMenu->insertItem( i18n( "Configure &PlayObject..." ), this, SLOT( slotConfigPlayObject() ), 0, ID_CONF_PLAYOBJECT );

            m_pPopupMenu->insertSeparator();

            //FIXME bad form, test the pointers!
            m_pActionCollection->action( "options_configure_keybinding" )->plug( m_pPopupMenu );
            m_pActionCollection->action( "options_configure_global_keybinding" )->plug( m_pPopupMenu );
            m_pActionCollection->action( "options_configure" )->plug( m_pPopupMenu );

            m_pPopupMenu->insertSeparator();

            m_pPopupMenu->insertItem( i18n( "&Help" ), (QPopupMenu*)helpMenu() );

            m_pPopupMenu->insertSeparator();

            m_pActionCollection->action( "file_quit" )->plug( m_pPopupMenu );
        }

        m_pPopupMenu->setItemChecked( ID_REPEAT_TRACK, pApp->m_optRepeatTrack );
        m_pPopupMenu->setItemChecked( ID_REPEAT_PLAYLIST, pApp->m_optRepeatPlaylist );
        m_pPopupMenu->setItemChecked( ID_RANDOM_MODE, pApp->m_optRandomMode );

        m_pPopupMenu->setItemEnabled( ID_CONF_PLAYOBJECT, pApp->playObjectConfigurable() );



        if( int id = m_pPopupMenu->exec( e->globalPos() ) )
        {
            bool *ptr;

            //set various bool items if clicked
            switch( id )
            {
            case ID_REPEAT_TRACK:
                ptr = &pApp->m_optRepeatTrack;
                break;
            case ID_REPEAT_PLAYLIST:
                ptr = &pApp->m_optRepeatPlaylist;
                break;
            case ID_RANDOM_MODE:
                ptr = &pApp->m_optRandomMode;
                break;
            default:
                ptr = 0;
            }

            if( ptr != 0 )
            {
                *ptr = !m_pPopupMenu->isItemChecked( id );
            }
        }
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

    //bit wierd this switch, but it fits our substandard methods ;-)
    switch( pApp->m_optVisCurrent )
    {
    case 1:
        m_pVis = new DistortAnalyzer( this );
        break;
/*    //FIXME deactivated piggz' analyzer for now, since it's too similar to #1 (confusing the user)
    case 2:
        m_pVis = new BarAnalyzer2( this );
        break;*/
    case 2:
        m_pVis = new TurbineAnalyzer( this );
        break;
/*    // bitch's ditched for this release
    case 3:
        m_pVis = new SpectralShineAnalyzer( this );
        break;*/
    case 3:
        m_pVis = new XmasAnalyzer( this );
        break;
    case 4:
        m_pVis = new BlockAnalyzer( this );
        break;
    case 0:
    default: //so we don't have to remember how many vis's there are
        m_pVis = new BarAnalyzer( this );
        pApp->m_optVisCurrent = 0;
    }

    // we special-case the DistortAnalyzer, since it needs more height. yes, this ugly.. I need whipping
/*    if ( pApp->m_optVisCurrent == 1 )
    {
        m_pVis->setFixedSize( 168, 80 );
        m_pVis->move( 113, 20 );
    }
    else
    {*/
        m_pVis->setFixedSize( 168, 50 );
        m_pVis->move( 119, 45 );
//    }

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


void PlayerWidget::slotConfigPlayObject()
{
    if ( pApp->m_pPlayObject && !m_pPlayObjConfigWidget )
    {
        m_pPlayObjConfigWidget = new ArtsConfigWidget( pApp->m_pPlayObject->object(), this );
        connect( pApp->m_pPlayObject, SIGNAL( destroyed() ), m_pPlayObjConfigWidget, SLOT( deleteLater() ) );

        m_pPlayObjConfigWidget->show();
    }
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
