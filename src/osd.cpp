/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * osd.cpp:   Shows some text in a pretty way independent to the WM
 * begin:     Fre Sep 26 2003
 * copyright: (C) 2004 Christian Muehlhaeuser <chris@chris.de>
 *            (C) 2004-2006 Seb Ruiz <me@sebruiz.net>
 *            (C) 2004, 2005 Max Howell
 *            (C) 2005 Gábor Lehel <illissius@gmail.com>
 */

#include "amarok.h"
#include "amarokconfig.h"
#include "collectiondb.h"    //for albumCover location
#include "debug.h"
#include "enginecontroller.h"
#include "osd.h"
#include "playlist.h"        //if osdUsePlaylistColumns()
#include "playlistitem.h"    //ditto
#include "podcastbundle.h"
#include "qstringx.h"
#include "starmanager.h"

#include <kapplication.h>
#include <kpixmap.h>
#include <kpixmapeffect.h>
#include <kstandarddirs.h>   //locate

#include <qbitmap.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qvaluevector.h>

namespace ShadowEngine
{
    QImage makeShadow( const QPixmap &textPixmap, const QColor &bgColor );
}


#define MOODBAR_HEIGHT 20


OSDWidget::OSDWidget( QWidget *parent, const char *name )
        : QWidget( parent, name, WType_TopLevel | WNoAutoErase | WStyle_Customize | WX11BypassWM | WStyle_StaysOnTop )
        , m_duration( 2000 )
        , m_timer( new QTimer( this ) )
        , m_alignment( Middle )
        , m_screen( 0 )
        , m_y( MARGIN )
        , m_drawShadow( false )
        , m_translucency( false )
        , m_rating( 0 )
        , m_volume( false )
{
    setFocusPolicy( NoFocus );
    setBackgroundMode( NoBackground );
    unsetColors();

    connect( m_timer, SIGNAL(timeout()), SLOT(hide()) );
    connect( CollectionDB::instance(), SIGNAL( ratingChanged( const QString&, int ) ),
             this, SLOT( ratingChanged( const QString&, int ) ) );

    //or crashes, KWin bug I think, crashes in QWidget::icon()
    kapp->setTopWidget( this );
}

void
OSDWidget::show( const QString &text, QImage newImage )
{
#ifdef Q_WS_X11
    m_text = text;
    if ( !newImage.isNull() )
    {
        m_cover = newImage;
        int w = m_scaledCover.width();
        int h = m_scaledCover.height();
        m_scaledCover = m_cover.smoothScale(w, h);
    }
    show();
#else
    Q_UNUSED( text );
    Q_UNUSED( newImage );
#endif
}

void
OSDWidget::ratingChanged( const short rating )
{
    //m_text = '\n' + i18n( "Rating changed" );
    setRating( rating ); //Checks isEnabled() before doing anything

    if( useMoodbar() )
        OSDWidget::setMoodbar( EngineController::instance()->bundle() );
    if( isShown() )
        show();
}

void
OSDWidget::ratingChanged( const QString& path, int rating )
{
    const MetaBundle &currentTrack = EngineController::instance()->bundle();
    if( currentTrack.isFile() && currentTrack.url().path() == path )
        ratingChanged( rating );
}

void
OSDWidget::volChanged( unsigned char volume )
{
    if ( isEnabled() )
    {
        m_volume = true;
        m_newvolume = volume;
        m_text = m_newvolume ? i18n("Volume: %1%").arg( m_newvolume ) : i18n("Mute");

        show();
    }
}

void
OSDWidget::show() //virtual
{
#ifdef Q_WS_X11
    if ( !isEnabled() || m_text.isEmpty() )
        return;

    const uint M = fontMetrics().width( 'x' );

    const QRect oldGeometry = QRect( pos(), size() );
    const QRect newGeometry = determineMetrics( M );

    if( m_translucency && !isShown() || !newGeometry.intersects( oldGeometry ) )
        m_screenshot = QPixmap::grabWindow( qt_xrootwin(),
                newGeometry.x(), newGeometry.y(),
                newGeometry.width(), newGeometry.height() );


    else if( m_translucency )
    {
        const QRect unite = oldGeometry.unite( newGeometry );
        KPixmap pix = QPixmap::grabWindow( qt_xrootwin(), unite.x(), unite.y(), unite.width(), unite.height() );

        QPoint p = oldGeometry.topLeft() - unite.topLeft();
        bitBlt( &pix, p, &m_screenshot );

        m_screenshot.resize( newGeometry.size() );

        p = newGeometry.topLeft() - unite.topLeft();
        bitBlt( &m_screenshot, 0, 0, &pix, p.x(), p.y() );
    }

    if( newGeometry.width() > 0 && newGeometry.height() > 0 )
    {
        render( M, newGeometry.size() );
        setGeometry( newGeometry );
        QWidget::show();
        bitBlt( this, 0, 0, &m_buffer );

        if( m_duration ) //duration 0 -> stay forever
            m_timer->start( m_duration, true ); //calls hide()
    }
    else
        warning() << "Attempted to make an invalid sized OSD\n";
#endif
}

QRect
OSDWidget::determineMetrics( const uint M )
{
    // sometimes we only have a tiddly cover
    const QSize minImageSize = m_cover.size().boundedTo( QSize(100,100) );

    // determine a sensible maximum size, don't cover the whole desktop or cross the screen
    const QSize margin( (M + MARGIN) * 2, (M + MARGIN) * 2 ); //margins
    const QSize image = m_cover.isNull() ? QSize( 0, 0 ) : minImageSize;
    const QSize max = QApplication::desktop()->screen( m_screen )->size() - margin;

    // If we don't do that, the boundingRect() might not be suitable for drawText() (Qt issue N67674)
    m_text.replace( QRegExp(" +\n"), "\n" );
    // remove consecutive line breaks
    m_text.replace( QRegExp("\n+"), "\n" );

    // The osd cannot be larger than the screen
    QRect rect = fontMetrics().boundingRect( 0, 0,
            max.width() - image.width(), max.height(),
            AlignCenter | WordBreak, m_text );

    if( m_volume )
    {
        static const QString tmp = QString ("******").insert( 3,
            ( i18n("Volume: 100%").length() >= i18n("Mute").length() )?
            i18n("Volume: 100%") : i18n("Mute") );

        QRect tmpRect = fontMetrics().boundingRect( 0, 0,
            max.width() - image.width(), max.height() - fontMetrics().height(),
            AlignCenter | WordBreak, tmp );
        tmpRect.setHeight( tmpRect.height() + fontMetrics().height() / 2 );

        rect = tmpRect;
    }

    if( m_rating )
    {
        QPixmap* star = StarManager::instance()->getStar( 1, true );
        if( rect.width() < star->width() * 5 )
            rect.setWidth( star->width() * 5 ); //changes right edge position
        rect.setHeight( rect.height() + star->height() + M ); //changes bottom edge pos
    }

    if( useMoodbar() )
        rect.setHeight( rect.height() + MOODBAR_HEIGHT + M );

    if( !m_cover.isNull() )
    {
        const int availableWidth = max.width() - rect.width() - M; //WILL be >= (minImageSize.width() - M)

        m_scaledCover = m_cover.smoothScale(
                QMIN( availableWidth, m_cover.width() ),
                QMIN( rect.height(), m_cover.height() ),
                QImage::ScaleMin ); //this will force us to be with our bounds

        int shadowWidth = 0;
        if( m_drawShadow && !m_scaledCover.hasAlpha() &&
          ( m_scaledCover.width() > 22 || m_scaledCover.height() > 22 ) )
            shadowWidth = static_cast<uint>( m_scaledCover.width() / 100.0 * 6.0 );

        const int widthIncludingImage = rect.width()
                + m_scaledCover.width()
                + shadowWidth
                + M; //margin between text + image

        rect.setWidth( widthIncludingImage );
    }

    // expand in all directions by M
    rect.addCoords( -M, -M, M, M );

    const QSize newSize = rect.size();
    const QRect screen = QApplication::desktop()->screenGeometry( m_screen );
    QPoint newPos( MARGIN, m_y );

    switch( m_alignment )
    {
        case Left:
            break;

        case Right:
            newPos.rx() = screen.width() - MARGIN - newSize.width();
            break;

        case Center:
            newPos.ry() = (screen.height() - newSize.height()) / 2;

            //FALL THROUGH

        case Middle:
            newPos.rx() = (screen.width() - newSize.width()) / 2;
            break;
    }

    //ensure we don't dip below the screen
    if ( newPos.y() + newSize.height() > screen.height() - MARGIN )
        newPos.ry() = screen.height() - MARGIN - newSize.height();

    // correct for screen position
    newPos += screen.topLeft();

    return QRect( newPos, rect.size() );
}

void
OSDWidget::render( const uint M, const QSize &size )
{
    /// render with margin/spacing @param M and @param size

    QPoint point;
    QRect rect( point, size );

    // From qt sources
    const uint xround = (M * 200) / size.width();
    const uint yround = (M * 200) / size.height();

    {   /// apply the mask
        static QBitmap mask;

        mask.resize( size );
        mask.fill( Qt::black );

        QPainter p( &mask );
        p.setBrush( Qt::white );
        p.drawRoundRect( rect, xround, yround );
        setMask( mask );
    }

    QColor shadowColor;
    {
        int h,s,v;
        foregroundColor().getHsv( &h, &s, &v );
        shadowColor = v > 128 ? Qt::black : Qt::white;
    }

    int align = Qt::AlignCenter | WordBreak;

    m_buffer.resize( rect.size() );
    QPainter p( &m_buffer );

    if( m_translucency )
    {
        KPixmap background( m_screenshot );
        KPixmapEffect::fade( background, 0.80, backgroundColor() );
        p.drawPixmap( 0, 0, background );
    }
    else
        p.fillRect( rect, backgroundColor() );

    p.setPen( backgroundColor().dark() );
    p.drawRoundRect( rect, xround, yround );

    rect.addCoords( M, M, -M, -M );

    if( !m_cover.isNull() )
    {
        QRect r( rect );
        r.setTop( (size.height() - m_scaledCover.height()) / 2 );
        r.setSize( m_scaledCover.size() );

        if( !m_scaledCover.hasAlpha() && m_drawShadow &&
          ( m_scaledCover.width() > 22 || m_scaledCover.height() > 22 ) ) {
            // don't draw a shadow for eg, the Amarok icon
            QImage shadow;
            const uint shadowSize = static_cast<uint>( m_scaledCover.width() / 100.0 * 6.0 );

            const QString folder = Amarok::saveLocation( "covershadow-cache/" );
            const QString file = QString( "shadow_albumcover%1x%2.png" ).arg( m_scaledCover.width()  + shadowSize )
                                                                        .arg( m_scaledCover.height() + shadowSize );
            if ( QFile::exists( folder + file ) )
                shadow.load( folder + file );
            else {
                shadow.load( locate( "data", "amarok/images/shadow_albumcover.png" ) );
                shadow = shadow.smoothScale( m_scaledCover.width() + shadowSize, m_scaledCover.height() + shadowSize );
                shadow.save( folder + file, "PNG" );
            }

            QPixmap target;
            target.convertFromImage( shadow ); //FIXME slow
            copyBlt( &target, 0, 0, &m_scaledCover );
            m_scaledCover = target;
            r.setTop( (size.height() - m_scaledCover.height()) / 2 );
            r.setSize( m_scaledCover.size() );
        }

        p.drawPixmap( r.topLeft(), m_scaledCover );

        rect.rLeft() += m_scaledCover.width() + M;
    }

    if( m_volume )
    {
        QPixmap vol;
        vol = QPixmap( rect.width(), rect.height() + fontMetrics().height() / 4 );

        QPixmap buf( vol.size() );
        QRect r( rect );
        r.setLeft( rect.left() + rect.width() / 2 - vol.width() / 2 );
        r.setTop( size.height() / 2 - vol.height() / 2);

        KPixmap pixmapGradient;
        { // gradient
            QBitmap mask;
            mask.resize( vol.size() );
            mask.fill( Qt::black );

            QPainter p( &mask );
            p.setBrush( Qt::white );
            p.drawRoundRect ( 3, 3, vol.width() - 6, vol.height() - 6,
                M * 300 / vol.width(), 99 );
            p.end();

            pixmapGradient = QPixmap( vol.size() );
            KPixmapEffect::gradient( pixmapGradient, colorGroup().background(),
                colorGroup().highlight(), KPixmapEffect::EllipticGradient );
            pixmapGradient.setMask( mask );
        }

        if( m_translucency )
        {
            KPixmap background( m_screenshot );
            KPixmapEffect::fade( background, 0.80, backgroundColor() );
            bitBlt( &vol, -r.left(), -r.top(), &background );
        }
        else
            vol.fill( backgroundColor() );

        { // vol ( bg-alpha )
            static QBitmap mask;
            mask.resize( vol.size() );
            mask.fill( Qt::white );

            QPainter p( &mask );
            p.setBrush( Qt::black );
            p.drawRoundRect ( 1, 1, rect.width()-2, rect.height() + fontMetrics().height() / 4 - 2,
                M * 300 / vol.width(), 99 );
            p.setBrush( Qt::white );
            p.drawRoundRect ( 3, 3, vol.width() - 6, vol.height() - 6,
                M * 300 / vol.width(), 99 );
            p.end();
            vol.setMask( mask );
        }
        buf.fill( backgroundColor().dark() );

        const int offset = int( double( vol.width() * m_newvolume ) / 100 );

        bitBlt( &buf, 0, 0, &vol ); // bg
        bitBlt( &buf, 0, 0, &pixmapGradient, 0, 0, offset );

        p.drawPixmap( r.left(), r.top(), buf );
        m_volume = false;
    }

    QPixmap* star = StarManager::instance()->getStar( m_rating/2, true );
    int graphicsHeight = 0;

    if( useMoodbar() )
    {
        QPixmap moodbar
          = m_moodbarBundle.moodbar().draw( rect.width(), MOODBAR_HEIGHT );
        QRect r( rect );
        r.setTop( rect.bottom() - moodbar.height()
                  - (m_rating ? star->height() + M : 0) );
        graphicsHeight += moodbar.height() + M;

        p.drawPixmap( r.left(), r.top(), moodbar );
        m_moodbarBundle = MetaBundle();
    }

    if( m_rating > 0 )
    {
        QRect r( rect );

        //Align to center...
        r.setLeft(( rect.left() + rect.width() / 2 ) - star->width() * m_rating / 4 );
        r.setTop( rect.bottom() - star->height() );
        graphicsHeight += star->height() + M;

        bool half = m_rating%2;

        if( half )
        {
            QPixmap* halfStar = StarManager::instance()->getHalfStar( m_rating/2 + 1, true );
            p.drawPixmap( r.left() + star->width() * ( m_rating / 2 ), r.top(), *halfStar );
            star = StarManager::instance()->getStar( m_rating/2 + 1, true );
        }

        for( int i = 0; i < m_rating/2; i++ )
        {
            p.drawPixmap( r.left() + i * star->width(), r.top(), *star );
        }

        m_rating = 0;
    }

    rect.setBottom( rect.bottom() - graphicsHeight );

    if( m_drawShadow )
    {
        QPixmap pixmap( rect.size() + QSize(10,10) );
        pixmap.fill( Qt::black );
        pixmap.setMask( pixmap.createHeuristicMask( true ) );

        QPainter p2( &pixmap );
        p2.setFont( font() );
        p2.setPen( Qt::white );
        p2.setBrush( Qt::white );
        p2.drawText( QRect(QPoint(5,5), rect.size()), align , m_text );
        p2.end();

        p.drawImage( rect.topLeft() - QPoint(5,5), ShadowEngine::makeShadow( pixmap, shadowColor ) );
    }

    p.setPen( foregroundColor() );
    p.setFont( font() );
    p.drawText( rect, align, m_text );
    p.end();
}

bool
OSDWidget::event( QEvent *e )
{
    switch( e->type() )
    {
    case QEvent::ApplicationPaletteChange:
        if( !AmarokConfig::osdUseCustomColors() )
            unsetColors(); //use new palette's colours
        return true;
    case QEvent::Paint:
        bitBlt( this, 0, 0, &m_buffer );
        return true;
    default:
        return QWidget::event( e );
    }
}

void
OSDWidget::mousePressEvent( QMouseEvent* )
{
    hide();
}

void
OSDWidget::unsetColors()
{
    const QColorGroup c = QApplication::palette().active();

    setPaletteForegroundColor( c.highlightedText() );
    setPaletteBackgroundColor( c.highlight() );
}

void
OSDWidget::setScreen( int screen )
{
    const int n = QApplication::desktop()->numScreens();
    m_screen = (screen >= n) ? n-1 : screen;
}

bool
OSDWidget::useMoodbar( void )
{
  return (m_moodbarBundle.moodbar().state() == Moodbar::Loaded  &&
          AmarokConfig::showMoodbar() );
}

//////  OSDPreviewWidget below /////////////////////

#include <kcursor.h>
#include <kiconloader.h>
#include <klocale.h>

namespace Amarok
{
    QImage icon() { return QImage( KIconLoader().iconPath( "amarok", -KIcon::SizeHuge ) ); }
}

OSDPreviewWidget::OSDPreviewWidget( QWidget *parent )
        : OSDWidget( parent, "osdpreview" )
        , m_dragging( false )
{
    m_text = i18n( "OSD Preview - drag to reposition" );
    m_duration = 0;
    m_cover = Amarok::icon();
}

void OSDPreviewWidget::mousePressEvent( QMouseEvent *event )
{
    m_dragOffset = event->pos();

    if( event->button() == LeftButton && !m_dragging ) {
        grabMouse( KCursor::sizeAllCursor() );
        m_dragging = true;
    }
}


void OSDPreviewWidget::mouseReleaseEvent( QMouseEvent * /*event*/ )
{
    if( m_dragging )
    {
        m_dragging = false;
        releaseMouse();

        // compute current Position && offset
        QDesktopWidget *desktop = QApplication::desktop();
        int currentScreen = desktop->screenNumber( pos() );

        if( currentScreen != -1 ) {
            // set new data
            m_screen = currentScreen;
            m_y      = QWidget::y();

            emit positionChanged();
        }
    }
}


void OSDPreviewWidget::mouseMoveEvent( QMouseEvent *e )
{
    if( m_dragging && this == mouseGrabber() )
    {
        // Here we implement a "snap-to-grid" like positioning system for the preview widget

        const QRect screen      = QApplication::desktop()->screenGeometry( m_screen );
        const uint  hcenter     = screen.width() / 2;
        const uint  eGlobalPosX = e->globalPos().x() - screen.left();
        const uint  snapZone    = screen.width() / 24;

        QPoint destination = e->globalPos() - m_dragOffset - screen.topLeft();
        int maxY = screen.height() - height() - MARGIN;
        if( destination.y() < MARGIN ) destination.ry() = MARGIN;
        if( destination.y() > maxY ) destination.ry() = maxY;

        if( eGlobalPosX < (hcenter-snapZone) ) {
            m_alignment = Left;
            destination.rx() = MARGIN;
        }
        else if( eGlobalPosX > (hcenter+snapZone) ) {
            m_alignment = Right;
            destination.rx() = screen.width() - MARGIN - width();
        }
        else {
            const uint eGlobalPosY = e->globalPos().y() - screen.top();
            const uint vcenter     = screen.height()/2;

            destination.rx() = hcenter - width()/2;

            if( eGlobalPosY >= (vcenter-snapZone) && eGlobalPosY <= (vcenter+snapZone) )
            {
                m_alignment = Center;
                destination.ry() = vcenter - height()/2;
            }
            else m_alignment = Middle;
        }

        destination += screen.topLeft();

        move( destination );
    }
}



//////  Amarok::OSD below /////////////////////

#include "enginecontroller.h"
#include "metabundle.h"
#include <qregexp.h>

Amarok::OSD::OSD(): OSDWidget( 0 )
{
    connect( CollectionDB::instance(), SIGNAL( coverChanged( const QString&, const QString& ) ),
             this,                   SLOT( slotCoverChanged( const QString&, const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( imageFetched( const QString& ) ),
             this,                   SLOT( slotImageChanged( const QString& ) ) );
}

void
Amarok::OSD::show( const MetaBundle &bundle ) //slot
{
#ifdef Q_WS_X11
    QString text = "";
    if( bundle.url().isEmpty() )
        text = i18n( "No track playing" );

    else
    {
        QValueVector<QString> tags;
        tags.append(bundle.prettyTitle());
        for( int i = 0; i < PlaylistItem::NUM_COLUMNS; ++i )
            tags.append(bundle.prettyText( i ));

        if( bundle.length() <= 0 )
            tags[PlaylistItem::Length+1] = QString::null;

        if( AmarokConfig::osdUsePlaylistColumns() )
        {
            QString tag;
            QValueVector<int> availableTags; //eg, ones that aren't empty
            static const QValueList<int> parens = //display these in parentheses
                QValueList<int>() << PlaylistItem::PlayCount  << PlaylistItem::Year   << PlaylistItem::Comment
                                  << PlaylistItem::Genre      << PlaylistItem::Length << PlaylistItem::Bitrate
                                  << PlaylistItem::LastPlayed << PlaylistItem::Score  << PlaylistItem::Filesize;
            OSDWidget::setMoodbar();
            OSDWidget::setRating( 0 );
            for( int i = 0, n = Playlist::instance()->numVisibleColumns(); i < n; ++i )
            {
                const int column = Playlist::instance()->mapToLogicalColumn( i );
                if( !tags.at( column + 1 ).isEmpty() && column != PlaylistItem::Rating )
                    availableTags.append(column);
                if( column == PlaylistItem::Rating )
                    OSDWidget::setRating( bundle.rating() );
                else if( column == PlaylistItem::Mood )
                    OSDWidget::setMoodbar( bundle );
            }

            for( int n = availableTags.count(), i = 0; i < n; ++i )
            {
                const int column = availableTags.at( i );
                QString append = ( i == 0 ) ? ""
                               : ( n > 1 && i == n / 2 ) ? "\n"
                               : ( parens.contains( column ) || parens.contains( availableTags.at( i - 1 ) ) ) ? " "
                               : i18n(" - ");
                append += ( parens.contains( column ) ? "(%1)" : "%1" );
                text += append.arg( tags.at( column + 1 ) );
            }
        }
        else
        {
            QMap<QString, QString> args;
            args["prettytitle"] = bundle.prettyTitle();
            for( int i = 0; i < PlaylistItem::NUM_COLUMNS; ++i )
                args[bundle.exactColumnName( i ).lower()] = bundle.prettyText( i );

            if( bundle.length() <= 0 )
                args["length"] = QString::null;


            uint time=EngineController::instance()->engine()->position();
            uint sec=(time/1000)%60;	//is there a better way to calculate the time?
            time /= 1000;
            uint min=(time/60)%60;
            time /= 60;
            uint hour=(time/60)%60;
            QString timeformat="";
            if(hour!=0)
            {
       	        timeformat += QString::number(hour);
                timeformat +=":";
            }
            timeformat +=QString::number(min);
            timeformat +=":";
            if(sec<10)
                timeformat +="0";
            timeformat +=QString::number(sec);
            args["elapsed"]=timeformat;
            QStringx osd = AmarokConfig::osdText();

            // hacky, but works...
            if( osd.contains( "%rating" ) )
                OSDWidget::setRating( AmarokConfig::useRatings() ? bundle.rating() : 0 );
            else
                OSDWidget::setRating( 0 );

            osd.replace( "%rating", "" );

            if( osd.contains( "%moodbar" )  &&  AmarokConfig::showMoodbar() )
                OSDWidget::setMoodbar( bundle );
            osd.replace( "%moodbar", "" );

            text = osd.namedOptArgs( args );

            // KDE 3.3 rejects \n in the .kcfg file, and KConfig turns \n into \\n, so...
            text.replace( "\\n", "\n" );
        }

        if ( AmarokConfig::osdCover() ) {
            //avoid showing the generic cover.  we can overwrite this by passing an arg.
            //get large cover for scaling if big cover needed

            QString location = QString::null;
            if( bundle.podcastBundle() )
                location = CollectionDB::instance()->podcastImage( bundle, false, 0 );
            else
                location = CollectionDB::instance()->albumImage( bundle, false, 0 );

            if ( location.find( "nocover" ) != -1 )
                setImage( Amarok::icon() );
            else
                setImage( location );
        }

        text = text.stripWhiteSpace();
    }

    if( text.isEmpty() )
        text = MetaBundle::prettyTitle( bundle.url().fileName() ).stripWhiteSpace();

    if( text.startsWith( "- " ) ) //When we only have a title tag, _something_ prepends a fucking hyphen. Remove that.
        text = text.mid( 2 );

    if( text.isEmpty() ) //still
        text = i18n("No information available for this track");

    OSDWidget::show( text );
#else
    Q_UNUSED( bundle );
#endif
}

void
Amarok::OSD::applySettings()
{
    setAlignment( static_cast<OSDWidget::Alignment>( AmarokConfig::osdAlignment() ) );
    setDuration( AmarokConfig::osdDuration() );
#ifdef Q_WS_X11
    setEnabled( AmarokConfig::osdEnabled() );
#else
    setEnabled( false );
#endif
    setOffset( AmarokConfig::osdYOffset() );
    setScreen( AmarokConfig::osdScreen() );
    setFont( AmarokConfig::osdFont() );
    setDrawShadow( AmarokConfig::osdDrawShadow() );
    setTranslucency( AmarokConfig::osdUseFakeTranslucency() );

    if( AmarokConfig::osdUseCustomColors() )
    {
        setTextColor( AmarokConfig::osdTextColor() );
        setBackgroundColor( AmarokConfig::osdBackgroundColor() );
    }
    else unsetColors();
}

void
Amarok::OSD::forceToggleOSD()
{
#ifdef Q_WS_X11
    if ( !isShown() ) {
        const bool b = isEnabled();
        setEnabled( true );
        show( EngineController::instance()->bundle() );
        setEnabled( b );
    }
    else
        hide();
#endif
}

void
Amarok::OSD::slotCoverChanged( const QString &artist, const QString &album )
{
    if( AmarokConfig::osdCover() && artist == EngineController::instance()->bundle().artist()
                                 && album  == EngineController::instance()->bundle().album()  )
    {
        QString location = CollectionDB::instance()->albumImage( artist, album, false, 0 );

        if( location.find( "nocover" ) != -1 )
            setImage( Amarok::icon() );
        else
            setImage( location );
    }
}

void
Amarok::OSD::slotImageChanged( const QString &remoteURL )
{
    QString url = EngineController::instance()->bundle().url().url();
    PodcastEpisodeBundle peb;
    if( CollectionDB::instance()->getPodcastEpisodeBundle( url, &peb ) )
    {
        PodcastChannelBundle pcb;
        if( CollectionDB::instance()->getPodcastChannelBundle( peb.parent().url(), &pcb ) )
        {
            if( pcb.imageURL().url() == remoteURL )
            {
                QString location = CollectionDB::instance()->podcastImage( remoteURL, false, 0 );
                if( location == CollectionDB::instance()->notAvailCover( false, 0 ) )
                    setImage( Amarok::icon() );
                else
                    setImage( location );
            }
        }
    }
}


/* Code copied from kshadowengine.cpp
 *
 * Copyright (C) 2003 Laur Ivan <laurivan@eircom.net>
 *
 * Many thanks to:
 *  - Bernardo Hung <deciare@gta.igs.net> for the enhanced shadow
 *    algorithm (currently used)
 *  - Tim Jansen <tim@tjansen.de> for the API updates and fixes.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

namespace ShadowEngine
{
    // Not sure, doesn't work above 10
    static const int    MULTIPLICATION_FACTOR = 3;
    // Multiplication factor for pixels directly above, under, or next to the text
    static const double AXIS_FACTOR = 2.0;
    // Multiplication factor for pixels diagonal to the text
    static const double DIAGONAL_FACTOR = 0.1;
    // Self explanatory
    static const int    MAX_OPACITY = 200;

    double decay( QImage&, int, int );

    QImage makeShadow( const QPixmap& textPixmap, const QColor &bgColor )
    {
        QImage result;

        const int w   = textPixmap.width();
        const int h   = textPixmap.height();
        const int bgr = bgColor.red();
        const int bgg = bgColor.green();
        const int bgb = bgColor.blue();

        int alphaShadow;

        // This is the source pixmap
        QImage img = textPixmap.convertToImage().convertDepth( 32 );

        result.create( w, h, 32 );
        result.fill( 0 ); // fill with black
        result.setAlphaBuffer( true );

        static const int M = 5;
        for( int i = M; i < w - M; i++) {
            for( int j = M; j < h - M; j++ )
            {
                alphaShadow = (int) decay( img, i, j );

                result.setPixel( i,j, qRgba( bgr, bgg , bgb, QMIN( MAX_OPACITY, alphaShadow ) ) );
            }
        }

        return result;
    }

    double decay( QImage& source, int i, int j )
    {
        //if ((i < 1) || (j < 1) || (i > source.width() - 2) || (j > source.height() - 2))
        //    return 0;

        double alphaShadow;
        alphaShadow =(qGray(source.pixel(i-1,j-1)) * DIAGONAL_FACTOR +
                qGray(source.pixel(i-1,j  )) * AXIS_FACTOR +
                qGray(source.pixel(i-1,j+1)) * DIAGONAL_FACTOR +
                qGray(source.pixel(i  ,j-1)) * AXIS_FACTOR +
                0                         +
                qGray(source.pixel(i  ,j+1)) * AXIS_FACTOR +
                qGray(source.pixel(i+1,j-1)) * DIAGONAL_FACTOR +
                qGray(source.pixel(i+1,j  )) * AXIS_FACTOR +
                qGray(source.pixel(i+1,j+1)) * DIAGONAL_FACTOR) / MULTIPLICATION_FACTOR;

        return alphaShadow;
    }
}

#include "osd.moc"
