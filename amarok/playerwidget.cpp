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

#include "playerwidget.h"
#include "viswidget.h"
#include "playerapp.h"
#include "browserwin.h"
#include "playlistwidget.h"
#include "effectwidget.h"

#include <qbitmap.h>
#include <qclipboard.h>
#include <qevent.h>
#include <qfont.h>
#include <qframe.h>
#include <qimage.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qrect.h>
#include <qslider.h>
#include <qstring.h>
#include <qstring.h>
#include <qthread.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <qwidget.h>

#include <kaction.h>
#include <kdebug.h>
#include <kglobalaccel.h>
#include <kkeydialog.h>
#include <klistview.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <ksystemtray.h>

#include <arts/artsgui.h>
#include <arts/connect.h>
#include <arts/dynamicrequest.h>
#include <arts/flowsystem.h>
#include <arts/kartswidget.h>
#include <arts/kmedia2.h>
#include <arts/kplayobjectfactory.h>
#include <arts/soundserver.h>


// CLASS AmarokButton ------------------------------------------------------------

AmarokButton::AmarokButton( QWidget *parent, QString activePixmap, QString inactivePixmap, bool toggleButton )
: QLabel( parent )

{
    m_activePixmap = QPixmap( activePixmap );
    m_inactivePixmap = QPixmap( inactivePixmap );
    m_isToggleButton = toggleButton;

    setOn( false );
    m_clicked = false;

    setBackgroundMode( Qt::FixedPixmap );
    setBackgroundOrigin( QWidget::WindowOrigin );
}



AmarokButton::~AmarokButton()
{
}



void AmarokButton::mousePressEvent( QMouseEvent *e )
{
    m_clicked = true;

    if ( m_isToggleButton )
    {
        setPixmap( m_activePixmap );
    }
    else
    {
        setOn( true );
    }
}



void AmarokButton::mouseReleaseEvent( QMouseEvent *e )
{
    if ( m_clicked )
    {
        if ( rect().contains( e->pos() ) )
        {
            if ( m_isToggleButton )
            {
                if ( m_on )
                    setOn( false);
                else
                    setOn( true );

                emit toggled( m_on );
            }
            else
            {
                setOn( false );
                emit clicked();
            }
        }
        else
        {
            setOn( false );
        }

    m_clicked = false;
    }
}



void AmarokButton::setOn( bool enable )
{
    if ( enable )
    {
        m_on = true;
        setPixmap( m_activePixmap );
    }
    else
    {
        m_on = false;
        setPixmap( m_inactivePixmap );
    }
}



bool AmarokButton::isOn()
{
    return m_on;
}



// CLASS AmarokSlider ------------------------------------------------------------

AmarokSlider::AmarokSlider( QWidget *parent ) : QSlider( parent )
{
}



AmarokSlider::~AmarokSlider()
{
}



void AmarokSlider::mousePressEvent( QMouseEvent *e )
{
    float newVal;

    if ( orientation() == QSlider::Horizontal )
        newVal = static_cast<float>( e->x() ) / static_cast<float>( width() ) * maxValue();
    else
        newVal = static_cast<float>( e->x() ) / static_cast<float>( height() ) * maxValue();
            
    int intVal = static_cast<int>( newVal );
            
    if ( ( intVal < value() - 10 ) || ( intVal > value() + 10 ) )
    {    
        pApp->m_bSliderIsPressed = true;
        setValue( intVal );
        emit sliderReleased();
    }

    QSlider::mousePressEvent( e );
}



// CLASS PlayerWidget ------------------------------------------------------------

PlayerWidget::PlayerWidget( QWidget *parent, const char *name ) : QWidget( parent, name )
{
    setName( "PlayerWidget " );
    setCaption( "amaroK" );
    setIcon( QPixmap( locate( "icon", "locolor/32x32/apps/amarok.png" ) ) );
    setFixedSize( 310, 165 );
    setPaletteForegroundColor( pApp->m_fgColor );
    m_pPopupMenu = NULL;
    m_pPlayObjConfigWidget = NULL;

    m_pActionCollection = new KActionCollection( this );

    m_oldBgPixmap.resize( size() );

    if ( paletteBackgroundPixmap() )
        m_oldBgPixmap = *paletteBackgroundPixmap();
    else
        m_oldBgPixmap.fill( pApp->m_bgColor );

    setPaletteBackgroundPixmap( QPixmap( locate( "data", "amarok/images/amaroKonlyHG_w320.jpg" ) ) );

    m_pVis = new VisWidget( this );
    m_pFrame = new QFrame( this );
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

    QString pathStr( locate( "data", "amarok/images/hi16-action-noatunback.png" ) );
    if ( pathStr == QString::null )
        QMessageBox::warning( this, "amaroK Error", "Error: Could not find icons. Did you forget make install?",
            QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );

    m_pButtonPrev = new QPushButton( m_pFrameButtons );
    m_pButtonPrev->setFocusPolicy( QWidget::NoFocus );
    m_pButtonPrev->setPixmap( locate( "data", "amarok/images/hi16-action-noatunback.png" ) );
    m_pButtonPrev->setFlat( true );

    m_pButtonPlay = new QPushButton( m_pFrameButtons );
    m_pButtonPlay->setFocusPolicy( QWidget::NoFocus );
    m_pButtonPlay->setPixmap( locate( "data", "amarok/images/hi16-action-noatunplay.png" ) );
    m_pButtonPlay->setToggleButton( true );
    m_pButtonPlay->setFlat( true );

    m_pButtonPause = new QPushButton( m_pFrameButtons );
    m_pButtonPause->setFocusPolicy( QWidget::NoFocus );
    m_pButtonPause->setPixmap( locate( "data", "amarok/images/hi16-action-noatunpause.png" ) );
    m_pButtonPause->setFlat( true );

    m_pButtonStop = new QPushButton( m_pFrameButtons );
    m_pButtonStop->setFocusPolicy( QWidget::NoFocus );
    m_pButtonStop->setPixmap( locate( "data", "amarok/images/hi16-action-noatunstop.png" ) );
    m_pButtonStop->setFlat( true );

    m_pButtonNext = new QPushButton( m_pFrameButtons );
    m_pButtonNext->setFocusPolicy( QWidget::NoFocus );
    m_pButtonNext->setPixmap( locate( "data", "amarok/images/hi16-action-noatunforward.png" ) );
    m_pButtonNext->setFlat( true );

    QBoxLayout* lay = new QVBoxLayout( this );
    lay->addWidget( m_pFrame );

    QBoxLayout *lay3 = new QHBoxLayout( lay );
    QBoxLayout *lay6 = new QVBoxLayout( lay3 );
    lay6->addItem( new QSpacerItem( 0, 2 ) );
    lay6->addWidget( m_pVis );

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
        locate( "data", "amarok/images/eq_inactive.png" ), true );
    lay4->addWidget( m_pButtonPl );
    lay4->addItem( new QSpacerItem( 0, 1 ) );
    lay4->addWidget( m_pButtonEq );
    lay4->addItem( new QSpacerItem( 2, 0 ) );

    lay->addItem( new QSpacerItem( 0, 5 ) );
    QBoxLayout* lay2 = new QHBoxLayout( lay );
    lay2->addWidget( m_pSlider );
    lay2->addWidget( m_pSliderVol );

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
    
    m_pFrame->setFixedSize( width(), 25 );
    m_pVis->setFixedSize( 168, 50 );
    m_pTimeDisplayLabel->setFixedSize( 9 * 12 + 2, 12 + 2 );

    // set up system tray
    m_pTray = new KSystemTray(this);
    m_pTray->setPixmap(pApp->miniIcon());
    m_pTray->show();
    QToolTip::add(m_pTray, i18n("amaroK media player"));

    initTimeDisplay();
    initScroll();
    timeDisplay( false, 0, 0, 0 );
}



PlayerWidget::~PlayerWidget()
{
}



// METHODS ----------------------------------------------------------------

void PlayerWidget::initScroll()
{
    m_pixmapWidth = 800;
    m_pixmapHeight = 20;

    m_pBgPixmap = new QPixmap( paletteBackgroundPixmap()->convertToImage().copy( m_pFrame->x(), m_pFrame->y(), m_pFrame->width(), m_pFrame->height() ) );

    m_pComposePixmap = new QPixmap( m_pFrame->width(), m_pixmapHeight );
    m_pScrollPixmap = new QPixmap( m_pixmapWidth, m_pixmapHeight );
    m_pScrollMask = new QBitmap( m_pixmapWidth, m_pixmapHeight );
    setScroll( "no file loaded", " ", " " );

    m_sx = m_sy = 0;
    m_sxAdd = 1;
}



void PlayerWidget::initTimeDisplay()
{
    m_timeDisplayX = 190;
    m_timeDisplayY = 84;
    m_timeDisplayW = 12;

    m_pTimePixmap = new QPixmap( locate( "data", "amarok/images/numbers_transp.png" ) );

    m_pTimeBgPixmap = new QPixmap( paletteBackgroundPixmap()->convertToImage()
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
    QToolTip::add(m_pTray, text);

    m_bitrate = bitrate;
    m_samplerate = samplerate;
    text.prepend( "   ***   " );

    m_pScrollMask->fill( Qt::color0 );
    QPainter painterPix( m_pScrollPixmap );
    QPainter painterMask( m_pScrollMask );
    painterPix.setBackgroundColor( Qt::black );
    painterPix.setPen( pApp->m_fgColor );
    painterMask.setPen( Qt::color1 );

    QFont font;
    font.setStyleHint( QFont::Helvetica );
    font.setFamily( "Helvetica" );
    font.setPointSize( 10 );
//  font.setBold( true );
    painterPix.setFont( font );
    painterMask.setFont( font );

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
        subs = -m_pFrame->width()+marginH;
        subs += dx + ( m_scrollWidth - sxTmp );
        if ( subs < 0 )
            subs = 0;
        bitBlt( m_pComposePixmap, dx, marginV,
            m_pScrollPixmap, sxTmp, m_sy, m_scrollWidth-sxTmp-subs, m_pixmapHeight, Qt::CopyROP );
        dx += ( m_scrollWidth - sxTmp );
        sxTmp += ( m_scrollWidth - sxTmp) ;

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
    bitBlt( m_pTimeComposePixmap, x, y, m_pTimePixmap, 10*m_timeDisplayW, 0, m_timeDisplayW );
    x += m_timeDisplayW;
    bitBlt( m_pTimeComposePixmap, x, y, m_pTimePixmap, seconds / 10 * m_timeDisplayW, 0, m_timeDisplayW );
    x += m_timeDisplayW;
    bitBlt( m_pTimeComposePixmap, x, y, m_pTimePixmap, seconds % 10 * m_timeDisplayW, 0, m_timeDisplayW );

                                                  //offset 1 pixel because of bounding box
    bitBlt( m_pTimeDisplayLabel, 1, 1, m_pTimeComposePixmap );
}



// EVENTS -----------------------------------------------------------------

void PlayerWidget::paintEvent( QPaintEvent * )
{
    erase( 20, 40, 120, 50 );

    QPainter pF( this );

    QFont font;
    font.setStyleHint( QFont::Helvetica );
    font.setFamily( "Helvetica" );
    font.setPointSize( 8 );
    pF.setFont( font );

    pF.setPen( pApp->m_fgColor );
    pF.drawText( 20, 40, m_bitrate );
    pF.drawText( 70, 40, m_samplerate );
}



void PlayerWidget::mouseReleaseEvent( QMouseEvent *e )
{
    if ( m_pTimeDisplayLabel->geometry().contains( e->pos() ) )
    {
        if ( pApp->m_optTimeDisplayRemaining )
            pApp->m_optTimeDisplayRemaining = false;
        else
            pApp->m_optTimeDisplayRemaining = true;
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

            m_pPopupMenu->insertItem( "About", pApp, SLOT( slotShowAbout() ) );
            m_pPopupMenu->insertItem( "amaroK Handbook", pApp, SLOT( slotShowHelp() ) );
            m_pPopupMenu->insertItem( "Tip of the Day", pApp, SLOT( slotShowTip() ) );

            m_pPopupMenu->insertSeparator();

            m_pPopupMenu->insertItem( "Settings", pApp, SLOT( slotShowOptions() ) );
            m_pPopupMenu->insertItem( "Configure Shortcuts", this, SLOT( slotConfigShortcuts() ) );
            m_pPopupMenu->insertItem( "Configure Global Shortcuts", this, SLOT( slotConfigGlobalShortcuts() ) );

            m_pPopupMenu->insertSeparator();

            m_pPopupMenu->insertItem( "Effects", pApp, SLOT( slotConfigEffects() ) );
            m_IdConfPlayObject = m_pPopupMenu->insertItem( "Configure PlayObject", this, SLOT( slotConfigPlayObject() ) );

            m_pPopupMenu->insertSeparator();

            m_IdRepeatTrack = m_pPopupMenu->insertItem( "Repeat Track", pApp, SLOT( slotSetRepeatTrack() ) );
            m_IdRepeatPlaylist = m_pPopupMenu->insertItem( "Repeat Playlist", pApp, SLOT( slotSetRepeatPlaylist() ) );

            m_pPopupMenu->insertSeparator();

            m_pPopupMenu->insertItem( "Quit", pApp, SLOT( quit() ) );
        }

        if ( playObjectConfigurable() )
            m_pPopupMenu->setItemEnabled( m_IdConfPlayObject, true );
        else
            m_pPopupMenu->setItemEnabled( m_IdConfPlayObject, false );

        m_pPopupMenu->exec( e->globalPos() );
    }
}



void PlayerWidget::closeEvent( QCloseEvent *e )
{
    if ( pApp->queryClose() )
        pApp->quit();
}



void PlayerWidget::moveEvent( QMoveEvent *e )
{

/*
 ** You can get the frame sizes like so (found in Qt sources while looking for something else):
framew = geometry().x() - x();
frameh = geometry().y() - y();
*/

/*!  Makes the the playlistwindow stick magnetically to the playerwindow */
/*    if ( pApp->m_pBrowserWin->isVisible() )
    {
        if ( ( e->oldPos().x() - 0 == pApp->m_pBrowserWin->frameGeometry().right() ) ||
             ( e->oldPos().y() - 0 == pApp->m_pBrowserWin->frameGeometry().bottom() ) ||
             ( e->oldPos().x() + frameSize().width() + 0 == pApp->m_pBrowserWin->frameGeometry().left() ) ||
             ( e->oldPos().y() + frameSize().height() + 0 == pApp->m_pBrowserWin->frameGeometry().top() ) )
        {
            pApp->m_pBrowserWin->move( e->pos() + ( pApp->m_pBrowserWin->pos() -  e->oldPos() ) );
        }
    }*/
}


// SLOTS ---------------------------------------------------------------------

void PlayerWidget::slotConfigShortcuts()
{
    KKeyDialog keyDialog( true );

    keyDialog.insert( m_pActionCollection, "Player Window" );
    keyDialog.insert( pApp->m_pBrowserWin->m_pActionCollection, "Playlist Window" );

    keyDialog.configure();
}



void PlayerWidget::slotConfigGlobalShortcuts()
{
    KKeyDialog::configure( pApp->m_pGlobalAccel, true, 0, true );
}



void PlayerWidget::slotCopyClipboard()
{
    QListViewItem *currentTrack = pApp->m_pBrowserWin->m_pPlaylistWidget->currentTrack();

    if ( currentTrack )
    {
        QClipboard *cb = QApplication::clipboard();
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



bool PlayerWidget::playObjectConfigurable()
{
    if ( pApp->m_pPlayObject && !m_pPlayObjConfigWidget )
    {
        Arts::TraderQuery query;
        query.supports( "Interface","Arts::GuiFactory" );
        query.supports( "CanCreate", pApp->m_pPlayObject->object()._interfaceName() );

        std::vector<Arts::TraderOffer> *queryResults = query.query();
        bool yes = queryResults->size();
        delete queryResults;

        return yes;
    }

    return false;
}

#include "playerwidget.moc"
