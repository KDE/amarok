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
#include "actionclasses.h"
#include "sliderwidget.h"
#include "analyzerbase.h"
#include "metabundle.h"      //setScroll()
#include "app.h"
#include "playerwindow.h"
#include "tracktooltip.h" //setScroll()
#include "enginecontroller.h"

#include <qdragobject.h>
#include <qfont.h>
#include <qhbox.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpopupmenu.h>
#include <qrect.h>
#include <qstringlist.h>
#include <qtooltip.h>

#include <kaction.h>
#include <kdebug.h>
#include <qevent.h> //various events
#include <klocale.h>
#include <kstandarddirs.h>
#include <ksystemtray.h>
#include <kmessagebox.h>
#include <kurldrag.h>
//#include <kwin.h> Yagami mode, most cool, try it!



//simple function for fetching amarok images
static inline QPixmap getPNG( const QString &filename )
{
    return QPixmap( locate( "data", QString( "amarok/images/%1.png" ).arg( filename ) ), "PNG" );
}


//fairly pointless template which was designed to make the ctor clearer,
//but probably achieves the opposite.
template<class W> static inline W*
createWidget( const QRect &r, QWidget *parent, const char *name = 0, Qt::WFlags f = 0 )
{
    W *w = new W( parent, name, f );
    w->setGeometry( r );
    return w;
}



PlayerWidget::PlayerWidget( QWidget *parent, const char *name, Qt::WFlags f )
    : QWidget( parent, name, f )
    , m_pAnimTimer( new QTimer( this ) )
    , m_pAnalyzer( 0 )
    , m_scrollBuffer( 291, 16 )
    , m_plusPixmap( getPNG( "time_plus" ) )
    , m_minusPixmap( getPNG( "time_minus" ) )
{
    //the createWidget template function is used here
    //createWidget just creates a widget which has it's geometry set too

    kdDebug() << "BEGIN " << k_funcinfo << endl;

    EngineController* const ec = EngineController::instance();

    ec->attach( this );

    setCaption( kapp->makeStdCaption( i18n("Player") ) );
    setFixedSize( 311, 140 );
    setAcceptDrops( true );
    setPaletteForegroundColor( Qt::white ); //0x80a0ff
    setPaletteBackgroundColor( QColor( 32, 32, 80 ) );

    QFont font;
    font.setBold( TRUE );
    font.setPixelSize( 10 );
    setFont( font );

    { //<NavButtons>
        //NOTE we use a layout for the buttons so resizing will be possible
        //TODO Plastik paints the small spaces inbetween buttons with ButtonColor and
        //     not backgroundColor. Report as bug.

        m_pFrameButtons = createWidget<QHBox>( QRect(0, 118, 311, 22), this );

        // In case you are wondering, the PLAY and PAUSE buttons are created here!

        //FIXME change the names of the icons to reflect kde names so we can fall back to them if necessary
        new NavButton( m_pFrameButtons, "prev", ec, SLOT( previous()  ) );
        m_pButtonPlay  = new NavButton( m_pFrameButtons, "play", ec, SLOT(play()) );
        m_pButtonPause = new NavButton( m_pFrameButtons, "pause", ec, SLOT(pause()) );
        new NavButton( m_pFrameButtons, "stop", ec, SLOT( stop()  ) );
        new NavButton( m_pFrameButtons, "next", ec, SLOT( next()  ) );

        m_pButtonPlay->setToggleButton( true );
        m_pButtonPause->setToggleButton( true );
    } //</NavButtons>

    { //<Sliders>
        m_pSlider    = new amaroK::Slider( this, Qt::Horizontal );
        m_pVolSlider = new amaroK::Slider( this, Qt::Vertical );

        m_pSlider->setGeometry( 4,103, 303,12 );
        m_pVolSlider->setGeometry( 294,18, 12,79 );

        m_pVolSlider->setMaxValue( amaroK::VOLUME_MAX );

        connect( m_pSlider,    SIGNAL( sliderReleased() ),
                 this,         SLOT  ( slotSliderReleased() ) );
        connect( m_pSlider,    SIGNAL( valueChanged( int ) ),
                 this,         SLOT  ( slotSliderChanged( int ) ) );
        connect( m_pVolSlider, SIGNAL( valueChanged( int ) ),
                 EngineController::instance(), SLOT  ( setVolume( int ) ) );
    } //<Sliders>

    { //<Scroller>
        font.setPixelSize( 11 );
        const int fontHeight = QFontMetrics( font ).height(); //the real height is more like 13px

        m_pScrollFrame = createWidget<QFrame>( QRect(6,18, 285,fontHeight), this );
        m_pScrollFrame->setFont( font );

        m_scrollBuffer.fill( backgroundColor() );
    { //</Scroller>

    } //<TimeLabel>
        font.setPixelSize( 18 );

        m_pTimeLabel = createWidget<QLabel>( QRect(16,36, 9*12+2,18), this, 0, Qt::WRepaintNoErase );
        m_pTimeLabel->setFont( font );

        m_timeBuffer.resize( m_pTimeLabel->size() );
        m_timeBuffer.fill( backgroundColor() );
    } //<TimeLabel>


    connect( m_pAnimTimer, SIGNAL( timeout() ), this, SLOT( drawScroll() ) );


    m_pButtonEq = new IconButton( this, "eq" );
    m_pButtonEq->setGeometry( 34,85, 28,13 );
    connect( m_pButtonEq, SIGNAL( released() ), this, SIGNAL( effectsWindowActivated() ) );

    m_pButtonPl = new IconButton( this, "pl" );
    m_pButtonPl->setGeometry( 5,85, 28,13 );
    connect( m_pButtonPl, SIGNAL( toggled( bool ) ), this, SIGNAL( playlistToggled( bool ) ) );


    m_pDescription = createWidget<QLabel>( QRect(4,6, 130,10), this );
    m_pTimeSign    = createWidget<QLabel>( QRect(6,40, 10,10), this, 0, Qt::WRepaintNoErase );
    m_pVolSign     = createWidget<QLabel>( QRect(295,7, 9,8),  this );

    m_pDescription->setPixmap( getPNG( "description" ) );
    m_pVolSign    ->setPixmap( getPNG( "vol_speaker" ) );

    defaultScroll();

    //Yagami mode!
    //KWin::setState( winId(), NET::KeepBelow | NET::SkipTaskbar | NET::SkipPager );
    //KWin::setType( winId(), NET::Override );
    //KWin::setOnAllDesktops( winId(), true );

    kdDebug() << "END " << k_funcinfo << endl;
}

PlayerWidget::~PlayerWidget()
{
    EngineController::instance()->detach( this );

    AmarokConfig::setPlayerPos( pos() );
}

// METHODS ----------------------------------------------------------------

void PlayerWidget::defaultScroll()
{
    m_rateString = QString::null;
    setScroll( i18n( "Welcome to amaroK" ) );
    m_pTimeLabel->hide();
    m_pTimeSign->hide();
}


void PlayerWidget::setScroll( const MetaBundle &bundle )
{
    QStringList text;

    text += bundle.prettyTitle();
    text += bundle.album();
    text += bundle.prettyLength();

    m_rateString = bundle.prettyBitrate();
    if( !m_rateString.isEmpty() && !bundle.prettySampleRate().isEmpty() ) m_rateString += " / ";
    m_rateString += bundle.prettySampleRate();
    if( m_rateString == " / " ) m_rateString = QString::null; //FIXME

    setScroll( text );

    //update image tooltip
    PlaylistToolTip::add( m_pScrollFrame, bundle );
}


void PlayerWidget::setScroll( const QStringList &list )
{
//#define MAX_KNOWS_BEST
#ifdef  MAX_KNOWS_BEST
static const char* const separator_xpm[]={
"5 5 2 1",
"# c #80a0ff",
". c none",
"#####",
"#...#",
"#...#",
"#...#",
"#####"};
#else
static const char* const separator_xpm[]={
"4 4 1 1",
"# c #80a0ff",
"####",
"####",
"####",
"####"};
#endif
    //TODO make me pretty!

    QPixmap separator( const_cast< const char** >(separator_xpm) );

    const QString s = list.first();

    //WARNING! don't pass an empty StringList to this function!

    QString text;
    QStringList list2( list );

    for( QStringList::Iterator it = list2.begin();
         it != list2.end(); )
    {
        if( (*it).isEmpty() ) it = list2.remove( it );
        else
        {
            text.append( *it );
            ++it;
        }
    }

    //FIXME WORKAROUND prevents crash
    if ( text.isEmpty() )
        text = "This message should not be displayed! Please report it to amarok-devel@lists.sf.net, thanks!";

    QFont font( m_pScrollFrame->font() );
    QFontMetrics fm( font );
    const uint separatorWidth = 21;
    const uint baseline = font.pixelSize(); //the font actually extends below its pixelHeight
    #ifdef MAX_KNOWS_BEST
    const uint separatorYPos = baseline - fm.boundingRect( "x" ).height();
    #else
    const uint separatorYPos = baseline - fm.boundingRect( "x" ).height() + 1;
    #endif
    m_scrollTextPixmap.resize( fm.width( text ) + list2.count() * separatorWidth, m_pScrollFrame->height() );
    m_scrollTextPixmap.fill( backgroundColor() );
    QPainter p( &m_scrollTextPixmap );
    p.setPen( foregroundColor() );
    p.setFont( font );
    uint x = 0;

    for( QStringList::ConstIterator it = list2.constBegin();
         it != list2.end();
         ++it )
    {
        p.drawText( x, baseline, *it );
        x += fm.width( *it );
        p.drawPixmap( x + 8, separatorYPos, separator );
        x += separatorWidth;
    }

     m_pTimeLabel->show(); m_pTimeSign->show();
    drawScroll();
    update(); //we need to update rateString
}


void PlayerWidget::drawScroll()
{
    static uint phase = 0;

    QPixmap* const buffer = &m_scrollBuffer;
    QPixmap* const scroll = &m_scrollTextPixmap;

    const uint topMargin  = 0; //moved margins into widget placement
    const uint leftMargin = 0; //as this makes it easier to fiddle
    const uint w = m_scrollTextPixmap.width();
    const uint h = m_scrollTextPixmap.height();

    phase += SCROLL_RATE;
    if( phase >= w ) phase = 0;

    int subs = 0;
    int dx = leftMargin;
    uint phase2 = phase;

    while( dx < m_pScrollFrame->width() )
    {
        subs = -m_pScrollFrame->width() + topMargin;
        subs += dx + ( w - phase2 );
        if( subs < 0 ) subs = 0;

        bitBlt( buffer, dx, topMargin,
                scroll, phase2, 0, w - phase2 - subs, h, Qt::CopyROP );

        dx     += ( w - phase2 );
        phase2 += ( w - phase2 );

        if( phase2 >= w ) phase2 = 0;
    }

    bitBlt( m_pScrollFrame, 0, 0, buffer );
}

// passive set. No signals are emited when using this toggle.
void PlayerWidget::setPlaylistShown( bool on )
{
    m_pButtonPl->blockSignals( true );
    m_pButtonPl->setOn( on );
    m_pButtonPl->blockSignals( false );
}


// passive set. No signals are emited when using this toggle.
void PlayerWidget::setEffectsWindowShown( bool on )
{
    m_pButtonEq->blockSignals( true );
    m_pButtonEq->setOn( on );
    m_pButtonEq->blockSignals( false );
}


void PlayerWidget::engineStateChanged( EngineBase::EngineState state )
{
    switch( state )
    {
        case EngineBase::Empty:
        case EngineBase::Idle:
            m_pButtonPlay->setOn( false );
            m_pButtonPause->setOn( false );

            defaultScroll();
            timeDisplay( 0 );
            m_pSlider->setValue( 0 );
            m_pSlider->setMaxValue( 0 );
        break;

        case EngineBase::Playing:
            m_pButtonPlay->setOn( true );
            m_pButtonPause->setOn( false );
            break;

        case EngineBase::Paused:
            m_pButtonPause->setOn( true );
            break;
    }
}

void PlayerWidget::engineVolumeChanged( int percent )
{
    if( !m_pVolSlider->sliding() )
    {
        m_pVolSlider->blockSignals( true );
        m_pVolSlider->setValue( percent );
        m_pVolSlider->blockSignals( false );
    }
}

void PlayerWidget::engineNewMetaData( const MetaBundle &bundle, bool trackChanged )
{
    if( trackChanged )
    {
        // we are playing
        m_pButtonPlay->setOn( true );
        m_pButtonPause->setOn( false );
        m_pSlider->setMaxValue( bundle.length() * 1000 );
    }

    setScroll( bundle );
}

void PlayerWidget::engineTrackPositionChanged( long position )
{
    if( isVisible() && !m_pSlider->sliding() )
    {
        m_pSlider->setValue( position );
        timeDisplay( position / 1000 );
    }
}


void PlayerWidget::slotSliderReleased()
{
    EngineBase *engine = EngineController::instance()->engine();
    if ( engine->state() == EngineBase::Playing )
    {
        engine->seek( m_pSlider->value() );
    }
}


void PlayerWidget::slotSliderChanged( int value )
{
    if( m_pSlider->sliding() )
    {
        value /= 1000;    // ms -> sec

        timeDisplay( value );
    }
}

void PlayerWidget::timeDisplay( int seconds )
{
    int songLength = EngineController::instance()->trackLength() / 1000;
    bool remaining = AmarokConfig::timeDisplayRemaining() && songLength > 0;

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
    p.setFont( m_pTimeLabel->font() );
    p.drawText( 0, 16, str ); //FIXME remove padding here and put in the widget placement!
    bitBlt( m_pTimeLabel, 0, 0, &m_timeBuffer );

    m_pTimeSign->setPixmap( remaining ? m_minusPixmap : m_plusPixmap );
}


// EVENTS -----------------------------------------------------------------

void PlayerWidget::paintEvent( QPaintEvent * )
{
    QPainter pF( this );
    //uses widget's font and foregroundColor() - see ctor
    pF.drawText( 6, 68, m_rateString );

    bitBlt( m_pScrollFrame, 0, 0, &m_scrollBuffer );
    bitBlt( m_pTimeLabel, 0, 0, &m_timeBuffer );
}


void PlayerWidget::wheelEvent( QWheelEvent *e )
{
    e->accept();

    switch( e->state() )
    {
    case ShiftButton:

        if( e->delta() > 0 )
            EngineController::instance()->previous();
        else
            EngineController::instance()->next();

        break;

    default:
        EngineController::instance()->setVolume( AmarokConfig::masterVolume() + e->delta() / 18 );
        pApp->slotShowVolumeOsd();
    }
}


void PlayerWidget::mousePressEvent( QMouseEvent *e )
{
    if ( e->button() == QMouseEvent::RightButton )
    {
        amaroK::Menu popup( this );
        popup.exec( e->globalPos() );
    }
    else //other buttons
    {
        QRect
        rect  = m_pTimeLabel->geometry();
        rect |= m_pTimeSign->geometry();

        if ( rect.contains( e->pos() ) )
        {
            AmarokConfig::setTimeDisplayRemaining( !AmarokConfig::timeDisplayRemaining() );
            repaint( true );
        }
        else if( m_pAnalyzer->geometry().contains( e->pos() ) )
        {
            createAnalyzer( e->state() & Qt::ControlButton ? -1 : +1 );
        }
        else startDrag(); //FIXME needs dragDelay
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


void PlayerWidget::dragEnterEvent( QDragEnterEvent *e )
{
   e->accept( KURLDrag::canDecode( e ) );
}


void PlayerWidget::dropEvent( QDropEvent *e )
{
    KURL::List list;
    if( KURLDrag::decode( e, list ) )
    {
        pApp->insertMedia( list );
    }
    else e->ignore();
}

void PlayerWidget::showEvent( QShowEvent * )
{
    m_pAnimTimer->start( ANIM_TIMER );
}


void PlayerWidget::hideEvent( QHideEvent * )
{
    m_pAnimTimer->stop();
}


// SLOTS ---------------------------------------------------------------------

void PlayerWidget::createAnalyzer( int increment )
{
    AmarokConfig::setCurrentAnalyzer( AmarokConfig::currentAnalyzer() + increment );

    delete m_pAnalyzer;

    m_pAnalyzer = Analyzer::Factory::createAnalyzer( this );
    m_pAnalyzer->setGeometry( 119,40, 168,56 );
    QToolTip::add( m_pAnalyzer, i18n( "Click for more analyzers" ) );
    m_pAnalyzer->show();
}


void PlayerWidget::startDrag()
{
    //TODO allow minimum drag distance

    QDragObject *d = new QTextDrag( pApp->dcopHandler()->nowPlaying(), this );
    d->dragCopy();
    // do NOT delete d.
}




NavButton::NavButton( QWidget *parent, const QString &icon, QObject *receiver, const char *slot )
  : QPushButton( parent )
{
    QString up = QString( "b_%1" ).arg( icon );
    QString down = QString( "b_%1_down" ).arg( icon );

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
    , m_up(   getPNG( icon + "_active2" ) ) //TODO rename files better (like the right way round for one!)
    , m_down( getPNG( icon + "_inactive2" ) )
{
    //const char *signal = isToggleButton ? SIGNAL( toggled( bool ) ) : SIGNAL( clicked() );
    //connect( this, signal, receiver, slot );

    setToggleButton( /*isToggleButton*/ true );
}

void IconButton::drawButton( QPainter *p )
{
    p->drawPixmap( 0, 0, (isOn()||isDown()) ? m_down : m_up );
}

#include "playerwindow.moc"
