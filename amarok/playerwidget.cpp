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

#include "amarokconfig.h"
#include "amarokdcophandler.h" //FIXME
#include "amarokslider.h"
#include "amaroksystray.h"
#include "analyzerbase.h"
#include "effectwidget.h"    //in the popupmenu
#include "enginebase.h"
#include "metabundle.h"      //setScroll()
#include "playerapp.h"
#include "playerwidget.h"
#include "playlisttooltip.h" //setScroll()

#include <math.h> //updateAnalzyer()

#include <qfont.h>
#include <qhbox.h>
#include <qiconset.h>
#include <qpainter.h>
#include <qpopupmenu.h>
#include <qrect.h>
#include <qstringlist.h>
#include <qdragobject.h>

#include <kaction.h>
#include <kdebug.h>
#include <kglobalaccel.h>
#include <khelpmenu.h>
#include <kkeydialog.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <ksystemtray.h>
#include <kmessagebox.h>


inline QPixmap getPNG( const QString &filename ) { return QPixmap( locate( "data", QString( "amarok/images/" ) + filename ), "PNG" ); }


PlayerWidget::PlayerWidget( QWidget *parent, const char *name )
    : QWidget( parent, name )
    , m_pActionCollection( new KActionCollection( this ) )
    , m_pDcopHandler( new AmarokDcopHandler )
    , m_pTray( 0 )
    , m_analyzer( 0 )
    , m_fht( PlayerApp::SCOPE_SIZE ) //FIXME
    , m_helpMenu( new KHelpMenu( this, KGlobal::instance()->aboutData(), m_pActionCollection ) )
    , m_scrollBuffer( 291, 16 ) //FIXME check ctor params //FIXME use staic const members for params
    , m_plusPixmap( getPNG( "time_plus.png" ) )
    , m_minusPixmap( getPNG( "time_minus.png" ) )
{
    setCaption( "amaroK" );
    setFixedSize( 311, 140 );
    //TODO set using derived QColorGroup..
    setPaletteForegroundColor( Qt::white ); //0x80a0ff
    setPaletteBackgroundColor( QColor( 32, 32, 80 ) );

    QFont font( "Arial" ); //= AmarokConfig::playerWidgetFont();
    font.setBold( TRUE );
    font.setPixelSize( 10 );
    setFont( font );

    //actions
    //FIXME declare these in PlayerApp.cpp and have global action collection
    KStdAction::keyBindings( this, SLOT( slotConfigShortcuts() ), m_pActionCollection );
    KStdAction::keyBindings( this, SLOT( slotConfigGlobalShortcuts() ), m_pActionCollection,
                             "options_configure_global_keybinding"
                           )->setText( i18n( "Configure Global Shortcuts..." ) );
    KStdAction::preferences( pApp, SLOT( slotShowOptions() ), m_pActionCollection );
    KStdAction::quit( kapp, SLOT( quit() ), m_pActionCollection );

    { //<NavButtons>
        //NOTE we use a layout for the buttons so resizing will be possible
        m_pFrameButtons = wrapper<QHBox>( QRect(0, 118, 311, 22), this );

        //FIXME change the names of the icons to reflect kde names so we can fall back to them if necessary
        new NavButton( m_pFrameButtons, "prev",  pApp, SLOT( slotPrev()  ) );
        m_pButtonPlay  = new NavButton( m_pFrameButtons, "play",  pApp, SLOT( slotPlay()  ) );
        m_pButtonPause = new NavButton( m_pFrameButtons, "pause", pApp, SLOT( slotPause() ) );
        new NavButton( m_pFrameButtons, "stop",  pApp, SLOT( slotStop()  ) );
        new NavButton( m_pFrameButtons, "next",  pApp, SLOT( slotNext()  ) );

        m_pButtonPlay->setToggleButton( true );
    } //</NavButtons>

    { //<Sliders>
        m_pSlider    = placeWidget( new AmarokSlider( this, Qt::Horizontal ), QRect(4,103, 303,12) );
        m_pVolSlider = placeWidget( new AmarokSlider( this, Qt::Vertical ), QRect(294,18, 12,79) );

        m_pVolSlider->setMaxValue( VOLUME_MAX );

        //FIXME move these slots here!
        connect( m_pSlider,    SIGNAL( sliderPressed() ),
                 pApp,         SLOT  ( slotSliderPressed() ) );
        connect( m_pSlider,    SIGNAL( sliderReleased() ),
                 pApp,         SLOT  ( slotSliderReleased() ) );
        connect( m_pSlider,    SIGNAL( valueChanged( int ) ),
                 pApp,         SLOT  ( slotSliderChanged( int ) ) );
        connect( m_pVolSlider, SIGNAL( valueChanged( int ) ),
                 pApp,         SLOT  ( slotVolumeChanged( int ) ) );
    } //<Sliders>

    { //<Scroller>
        font.setPixelSize( 11 );
        int fontHeight = QFontMetrics( font ).height(); //the real height is more like 13px

        m_scrollFrame = wrapper<QFrame>( QRect(6,18, 285,fontHeight), this );
        m_scrollFrame->setFont( font );

        #ifdef USE_SCROLLMASK
        m_pScrollMask = new QBitmap( m_scrollFrame->size() );
        #endif
        m_scrollBuffer.fill( backgroundColor() );
      //</Scroller>

      //<TimeLabel>
        font.setPixelSize( 18 );

        m_timeLabel = wrapper<QLabel>( QRect(16,36, 9*12+2,16), this, 0, Qt::WRepaintNoErase );
        m_timeLabel->setFont( font );

        m_timeBuffer.resize( m_timeLabel->size() );
        m_timeBuffer.fill( backgroundColor() );
    } //<TimeLabel>

    m_pButtonEq = placeWidget( new IconButton( this, "eq" ), QRect(34,85, 28,13) );
    m_pButtonPl = placeWidget( new IconButton( this, "pl" ), QRect( 5,85, 28,13) );

    m_pDescription = wrapper<QLabel>( QRect(4,6, 130,10), this );
    m_pTimeSign    = wrapper<QLabel>( QRect(6,40, 10,10), this, 0, Qt::WRepaintNoErase );
    m_pVolSign     = wrapper<QLabel>( QRect(295,7, 9,8),  this );

    m_pDescription->setPixmap( getPNG( "description.png" ) );
    m_pVolSign    ->setPixmap( getPNG( "vol_speaker.png" ) );

    m_pTray = new AmarokSystray( this, m_pActionCollection ); //show/hide is handled by KConfig XT

    defaultScroll();

    connect( &m_analyzerTimer, SIGNAL( timeout() ), SLOT( updateAnalyzer() ) );
}


PlayerWidget::~PlayerWidget()
{
    m_analyzerTimer.stop();
}


// METHODS ----------------------------------------------------------------

void PlayerWidget::defaultScroll()
{
    m_rateString = QString::null;

    setScroll( i18n( "Welcome to amaroK" ) );

    QToolTip::remove( m_pTray );
    QToolTip::add( m_pTray, i18n( "amaroK - Audio Player" ) );
    m_pDcopHandler->setNowPlaying( QString::null );
}


void PlayerWidget::setScroll( const MetaBundle &bundle )
{
    QStringList text;

    text += bundle.prettyTitle();
    text += bundle.m_album;
    text += bundle.prettyLength();

    m_rateString = bundle.prettyBitrate();
    if( !m_rateString.isEmpty() ) m_rateString += " / ";
    m_rateString += bundle.prettySampleRate();
    if( m_rateString == " / " ) m_rateString = QString::null; //FIXME

    setScroll( text );

    //update image tooltip
    PlaylistToolTip::add( m_scrollFrame, bundle );
}


void PlayerWidget::setScroll( const QStringList &list )
{
static const char* const not_close_xpm[]={
"5 5 2 1",
"# c white",//#ffeacd",
". c #80a0ff",
"#####",
"#...#",
"#...#",
"#...#",
"#####"};

    QPixmap square( const_cast< const char** >(not_close_xpm) );

    const QString s = list.first();
    QToolTip::remove( m_pTray );
    QToolTip::add( m_pTray, s );
    m_pDcopHandler->setNowPlaying( s );

    //WARNING! don't pass an empty StringList to this function!

    QString text;
    QStringList list2( list );
    //const QString separator = " | ";

    for( QStringList::Iterator it = list2.begin();
         it != list2.end(); )
    {
        if( (*it).isEmpty() ) it = list2.remove( it );
        else
        {
            kdDebug() << *it << endl;
            text.append( *it );
            //text.append( separator );
            ++it;
        }
    }

    //FIXME WORKAROUND prevents crash
    if ( text.isEmpty() )
        text = "FIXME: EMPTY STRING MAKES ME CRASH";
    
    QFont font( m_scrollFrame->font() );
    QFontMetrics fm( font );
    const uint height = font.pixelSize(); //the font actually extends below its pixelHeight
    m_scrollTextPixmap.resize( fm.width( text ) + list2.count() * 20, m_scrollFrame->height() );
    //m_scrollTextPixmap.resize( fm.width( text ), m_scrollFrame->height() );
    m_scrollTextPixmap.fill( backgroundColor() );
    QPainter p( &m_scrollTextPixmap );
    p.setPen( foregroundColor() );
    p.setFont( font );
    uint x = 0;
    //const QColor separatorColor( Qt::lightGray );
    const uint   separatorWidth = 20;//fm.width( separator );

    for( QStringList::ConstIterator it = list2.constBegin();
         it != list2.end();
         ++it )
    {
        //p.setPen( foregroundColor() );
        p.drawText( x, height, *it );
        x += fm.width( *it );

        //p.setPen( separatorColor );
        //p.drawText( x, height, separator );
        p.drawPixmap( x + 8, font.pixelSize() / 2, square );
        x += separatorWidth;
    }

    drawScroll();
    update(); //we need to update rateString
}


void PlayerWidget::drawScroll()
{
    static uint phase = 0;

    QPixmap* const buffer = &m_scrollBuffer;
    QPixmap* const scroll = &m_scrollTextPixmap;

    const int topMargin  = 0; //moved margins into widget placement
    const int leftMargin = 0; //as this makes it easier to fiddle
    const int w = m_scrollTextPixmap.width();
    const int h = m_scrollTextPixmap.height();

    phase += SCROLL_RATE;
    if( phase >= w ) phase = 0;

    int subs = 0;
    int dx = leftMargin;
    int phase2 = phase;

    while( dx < m_scrollFrame->width() )
    {
        subs = -m_scrollFrame->width() + topMargin;
        subs += dx + ( w - phase2 );
        if( subs < 0 ) subs = 0;

        bitBlt( buffer, dx, topMargin,
                scroll, phase2, 0, w - phase2 - subs, h, Qt::CopyROP );

        dx     += ( w - phase2 );
        phase2 += ( w - phase2 );

        if( phase2 >= w ) phase2 = 0;
    }

    bitBlt( m_scrollFrame, 0, 0, buffer );
}


void PlayerWidget::timeDisplay( int seconds )
{
    int songLength = pApp->trackLength() / 1000;
    bool remaining = AmarokConfig::timeDisplayRemaining() && !pApp->m_pEngine->isStream() && songLength > 0;

    if( remaining ) seconds = songLength - seconds;

    QString
    str  = zeroPad( seconds /60/60%60 );
    str += ':';
    str += zeroPad( seconds /60%60 );
    str += ':';
    str += zeroPad( seconds %60 );

    m_timeBuffer.fill( backgroundColor() );
    QPainter p( &m_timeBuffer );
    p.setPen( foregroundColor() );
    p.setFont( m_timeLabel->font() );
    p.drawText( 0, 16, str ); //FIXME remove padding here and put in the widget placement!
    bitBlt( m_timeLabel, 0, 0, &m_timeBuffer );

    m_pTimeSign->setPixmap( remaining ? m_minusPixmap : m_plusPixmap );
}


// EVENTS -----------------------------------------------------------------

void PlayerWidget::paintEvent( QPaintEvent * )
{
    QPainter pF( this );
    //uses widget's font and foregroundColor() - see ctor
    pF.drawText( 6, 68, m_rateString );

    bitBlt( m_scrollFrame, 0, 0, &m_scrollBuffer );
    bitBlt( m_timeLabel, 0, 0, &m_timeBuffer ); //FIXME have functions that replace these blts that are inlined things like "bltTimeDisplay" etc.
}


void PlayerWidget::wheelEvent( QWheelEvent *e )
{
    e->accept();

    switch( e->state() )
    {
    case ShiftButton:

        if( e->delta() > 0 )
            pApp->slotPrev();
        else
            pApp->slotNext();

        break;

    default:

        AmarokConfig::setMasterVolume( AmarokConfig::masterVolume() + e->delta() / 18 );

        if ( AmarokConfig::masterVolume() < 0 )
            AmarokConfig::setMasterVolume( 0 );
        if ( AmarokConfig::masterVolume() > VOLUME_MAX )
            AmarokConfig::setMasterVolume( VOLUME_MAX );

        pApp->slotVolumeChanged( AmarokConfig::masterVolume() );
        m_pVolSlider->setValue( AmarokConfig::masterVolume() );
    }
}


void PlayerWidget::mousePressEvent( QMouseEvent *e )
{
    #define ID_REPEAT_TRACK 100
    #define ID_REPEAT_PLAYLIST 101
    #define ID_RANDOM_MODE 102
    #define ID_CONF_PLAYOBJECT 103

    if ( e->button() == QMouseEvent::RightButton )
    {
        QPopupMenu popup;
        popup.setCheckable( true );

        popup.insertItem( i18n( "Repeat &Track" ),    ID_REPEAT_TRACK );
        popup.insertItem( i18n( "Repeat Play&list" ), ID_REPEAT_PLAYLIST );
        popup.insertItem( i18n( "Random &Mode" ),     ID_RANDOM_MODE );
      popup.insertSeparator();
        popup.insertItem( i18n( "Configure &Effects..." ), m_pButtonEq, SLOT( toggle() ) );
        popup.insertItem( i18n( "Configure &PlayObject..." ), this, SLOT( slotConfigPlayObject() ), 0, ID_CONF_PLAYOBJECT );
      popup.insertSeparator();
        m_pActionCollection->action( "options_configure_keybinding" )->plug( &popup );
        m_pActionCollection->action( "options_configure_global_keybinding" )->plug( &popup );
        m_pActionCollection->action( "options_configure" )->plug( &popup );
      popup.insertSeparator();
        popup.insertItem( i18n( "&Help" ), (QPopupMenu*)m_helpMenu->menu() );
      popup.insertSeparator();
        m_pActionCollection->action( "file_quit" )->plug( &popup );

        popup.setItemChecked( ID_REPEAT_TRACK,    AmarokConfig::repeatTrack() );
        popup.setItemChecked( ID_REPEAT_PLAYLIST, AmarokConfig::repeatPlaylist() );
        popup.setItemChecked( ID_RANDOM_MODE,     AmarokConfig::randomMode() );
        popup.setItemEnabled( ID_CONF_PLAYOBJECT, pApp->playObjectConfigurable() );

        switch( popup.exec( e->globalPos() ) )
        {
        case ID_REPEAT_TRACK:
            AmarokConfig::setRepeatTrack( !popup.isItemChecked(ID_REPEAT_TRACK) );
            break;
        case ID_REPEAT_PLAYLIST:
            AmarokConfig::setRepeatPlaylist( !popup.isItemChecked(ID_REPEAT_PLAYLIST) );
            break;
        case ID_RANDOM_MODE:
            AmarokConfig::setRandomMode( !popup.isItemChecked(ID_RANDOM_MODE) );
            break;
        }
    }
    else //other buttons
    {
        QRect
        rect  = m_timeLabel->geometry();
        rect |= m_pTimeSign->geometry();

        if ( rect.contains( e->pos() ) )
        {
            AmarokConfig::setTimeDisplayRemaining( !AmarokConfig::timeDisplayRemaining() );
            repaint( true );
        }
        else if( dynamic_cast<QWidget*>(m_analyzer)->geometry().contains( e->pos() ) ) { createAnalyzer( true ); return; }
        else startDrag();
    }
}


void PlayerWidget::closeEvent( QCloseEvent *e )
{
    //KDE policy states we should hide to tray and not quit() when the close window button is
    //pushed for the main widget -mxcl
    //of course since we haven't got an obvious quit button, this is not yet a perfect solution..

    //NOTE we must accept() here or the info box below appears on quit()
    //Don't ask me why.. *shrug*

    e->accept();

    if( AmarokConfig::showTrayIcon() && !e->spontaneous() && !kapp->sessionSaving() /*&& !QApplication::closingDown()*/ )
    {
        KMessageBox::information( this,
                                  i18n( "<qt>Closing the main window will keep amaroK running in the system tray. "
                                        "Use Quit from the popup-menu to quit the application.</qt>" ),
                                  i18n( "Docking in System Tray" ), "hideOnCloseInfo" );
    }
    else kapp->quit();
}


// SLOTS ---------------------------------------------------------------------

void PlayerWidget::createAnalyzer( bool increment )
{
    if( increment ) AmarokConfig::setCurrentAnalyzer( AmarokConfig::currentAnalyzer() + 1 );

    delete m_analyzer;

    m_analyzer = AnalyzerBase::AnalyzerFactory::createAnalyzer( this );

    QWidget *visWidget = dynamic_cast<QWidget*>(m_analyzer); //we don't test for 0 as we know it'll work! :)
    if ( AmarokConfig::currentAnalyzer() == 1 ) //FIXME
    {
        visWidget->move( QPoint( 119, 30 ) );
        visWidget->resize( QSize( 168, 70 ) );
    }
    else
    {
        visWidget->move( QPoint( 119, 40 ) );
        visWidget->resize( QSize( 168, 56 ) );
    }
    visWidget->show();

    m_analyzerTimer.start( m_analyzer->timeout() );
}

#include "browserwin.h" //FIXME bah!
void PlayerWidget::slotConfigShortcuts() //FIXME move to pApp
{
    KKeyDialog keyDialog( true );

    keyDialog.insert( m_pActionCollection, i18n( "Player Window" ) );
    keyDialog.insert( pApp->m_pBrowserWin->m_pActionCollection, i18n( "Playlist Window" ) );

    keyDialog.configure();
}


void PlayerWidget::slotConfigGlobalShortcuts() //FIXME move to pApp
{
    KKeyDialog::configure( pApp->m_pGlobalAccel, true, 0, true );
}


void PlayerWidget::slotConfigPlayObject() //FIXME move to pApp
{
/*    if ( pApp->m_pPlayObject && !m_pPlayObjConfigWidget )
    {
        m_pPlayObjConfigWidget = new ArtsConfigWidget( pApp->m_pPlayObject->object(), this );
        connect( pApp->m_pPlayObject, SIGNAL( destroyed() ), m_pPlayObjConfigWidget, SLOT( deleteLater() ) );

        m_pPlayObjConfigWidget->show();
    }*/
}


void PlayerWidget::startDrag()
{
    //TODO allow minimum drag distance

    QDragObject *d = new QTextDrag( m_pDcopHandler->nowPlaying(), this );
    d->dragCopy();
    // do NOT delete d.
}


void PlayerWidget::updateAnalyzer()
{
    EngineBase *m_pEngine = pApp->m_pEngine;

    if ( !isVisible() )
        return; //FIXME pause all timers when hidden

    static int t = 201;

    if ( m_pEngine->state() == EngineBase::Playing )
    {
        std::vector<float> *pScopeVector = m_pEngine->scope();
        float *front = static_cast<float*>( &pScopeVector->front() );
        if (!front)
            return;

        if ( AmarokConfig::currentAnalyzer() == 2)
        { // sonogram
            m_fht.power( front );
            m_fht.scale( front, 1.0 / 64 );
        }
        else
        {
            float *f = new float[ m_fht.size() ];
            m_fht.copy( f, front );
            m_fht.logSpectrum( front, f );
            m_fht.scale( front, 1.0 / 20 );
            delete[] f;
        }
        pScopeVector->resize( pScopeVector->size() / 2 );

        m_analyzer->drawAnalyzer( pScopeVector );

        delete pScopeVector;
        //FIXME <markey> beat detection code temporarily moved to VIS_PLAN, since it was disabled anyway
    }
    else
    {
        if ( t > 999 ) t = 1; //0 = wasted calculations
        if ( t < 201 )
        {
            double dt = double(t) / 200 ;
            std::vector<float> v( 31 );
            for( uint i = 0; i < v.size(); ++i )
                v[i] = dt * (sin( M_PI + (i * M_PI) / v.size() ) + 1.0);
            m_analyzer->drawAnalyzer( &v );
        }
        else
            m_analyzer->drawAnalyzer( NULL );

        ++t;
    }
}



NavButton::NavButton( QWidget *parent, const QString &icon, QObject *receiver, const char *slot )
  : QPushButton( parent )
{
    QString up = QString( "b_%1.png" ).arg( icon );
    QString down = QString( "b_%1_down.png" ).arg( icon );

    QIconSet iconSet;
    iconSet.setPixmap( getPNG( up   ), QIconSet::Automatic, QIconSet::Normal, QIconSet::Off );
    iconSet.setPixmap( getPNG( down ), QIconSet::Automatic, QIconSet::Normal, QIconSet::On  );

    setIconSet( iconSet );
    setFocusPolicy( QWidget::NoFocus );
    setFlat( true );

    connect( this, SIGNAL( clicked() ), receiver, slot );
}



IconButton::IconButton( QWidget *parent, const QString &icon/*, QObject *receiver, const char *slot, bool isToggleButton*/ )
    : QButton( parent )
    , m_up(   getPNG( icon + "_active2.png" ) ) //TODO rename files better (like the right way round for one!)
    , m_down( getPNG( icon + "_inactive2.png" ) )
{
    //const char *signal = isToggleButton ? SIGNAL( toggled( bool ) ) : SIGNAL( clicked() );
    //connect( this, signal, receiver, slot );

    setToggleButton( /*isToggleButton*/ true );
}

void IconButton::drawButton( QPainter *p )
{
    p->drawPixmap( 0, 0, (isOn()||isDown()) ? m_down : m_up );
}

#include "playerwidget.moc"
