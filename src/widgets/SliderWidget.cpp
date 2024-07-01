/****************************************************************************************
 * Copyright (c) 2003-2009 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2005 Gabor Lehel <illissius@gmail.com>                                 *
 * Copyright (c) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
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

#include "SliderWidget.h"

#include <config.h>

#include "core/support/Amarok.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "amarokconfig.h"
#include "App.h"
#include "BookmarkTriangle.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "core/meta/support/MetaUtility.h"
#include "SvgHandler.h"
#include "ProgressWidget.h"

#include <KLocalizedString>

#include <QAction>
#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>

Amarok::Slider::Slider( Qt::Orientation orientation, uint max, QWidget *parent )
    : QSlider( orientation, parent )
    , m_sliding( false )
    , m_outside( false )
    , m_prevValue( 0 )
    , m_needsResize( true )
{
    setMouseTracking( true );
    setRange( 0, max );
    setAttribute( Qt::WA_NoMousePropagation, true );
    setAttribute( Qt::WA_Hover, true );
    if ( orientation == Qt::Vertical )
    {
        setInvertedAppearance( true );
        setInvertedControls( true );
    }
}

QRect
Amarok::Slider::sliderHandleRect( const QRect &slider, qreal percent ) const
{
    QRect rect;
    const bool inverse = ( orientation() == Qt::Horizontal ) ?
                         ( invertedAppearance() != (layoutDirection() == Qt::RightToLeft) ) :
                         ( !invertedAppearance() );

    if( m_usingCustomStyle)
        rect = The::svgHandler()->sliderKnobRect( slider, percent, inverse );
    else
    {
        if ( inverse )
            percent = 1.0 - percent;
        const int handleSize = style()->pixelMetric( QStyle::PM_SliderControlThickness );
        rect = QRect( 0, 0, handleSize, handleSize );
        rect.moveTo( slider.x() + qRound( ( slider.width() - handleSize ) * percent ), slider.y() + 1 );
    }

    return rect;
}

void
Amarok::Slider::wheelEvent( QWheelEvent *e )
{
    DEBUG_BLOCK

    if( orientation() == Qt::Vertical )
    {
        // Will be handled by the parent widget
        e->ignore();
        return;
    }

    // Position Slider (horizontal)
    // only used for progress slider now!
    int step = e->angleDelta().y() * 24; //FIXME: check if .x() must be used
    int nval = value() + step;
    nval = qMax(nval, minimum());
    nval = qMin(nval, maximum());

    QSlider::setValue( nval );

    Q_EMIT sliderReleased( value() );
}

void
Amarok::Slider::mouseMoveEvent( QMouseEvent *e )
{
    if ( m_sliding )
    {
        //feels better, but using set value of 20 is bad of course
        QRect rect( -20, -20, width()+40, height()+40 );

        if ( orientation() == Qt::Horizontal && !rect.contains( e->pos() ) )
        {
            if ( !m_outside )
            {
                QSlider::setValue( m_prevValue );
                //if mouse released outside of slider, Q_EMIT sliderMoved to previous value
                Q_EMIT sliderMoved( m_prevValue );
            }
            m_outside = true;
        }
        else
        {
            m_outside = false;
            slideEvent( e );
            Q_EMIT sliderMoved( value() );
        }
    }
    else
        QSlider::mouseMoveEvent( e );
}

void
Amarok::Slider::slideEvent( QMouseEvent *e )
{
    QRect knob;
    if ( maximum() > minimum() )
        knob = sliderHandleRect( rect(), ((qreal)value()) / ( maximum() - minimum() ) );

    int position;
    int span;

    if( orientation() == Qt::Horizontal )
    {
        position = e->pos().x() - knob.width() / 2;
        span = width() - knob.width();
    }
    else
    {
        position = e->pos().y() - knob.height() / 2;
        span = height() - knob.height();
    }

    const bool inverse = ( orientation() == Qt::Horizontal ) ?
                         ( invertedAppearance() != (layoutDirection() == Qt::RightToLeft) ) :
                         ( !invertedAppearance() );
    const int val = QStyle::sliderValueFromPosition( minimum(), maximum(), position, span, inverse );
    QSlider::setValue( val );
}

void
Amarok::Slider::mousePressEvent( QMouseEvent *e )
{
    m_sliding   = true;
    m_prevValue = value();

    QRect knob;
    if ( maximum() > minimum() )
        knob = sliderHandleRect( rect(), ((qreal)value()) / ( maximum() - minimum() ) );
    if ( !knob.contains( e->pos() ) )
        mouseMoveEvent( e );
}

void
Amarok::Slider::mouseReleaseEvent( QMouseEvent* )
{
    if( !m_outside && value() != m_prevValue )
       Q_EMIT sliderReleased( value() );

    m_sliding = false;
    m_outside = false;
}

void
Amarok::Slider::setValue( int newValue )
{
    //don't adjust the slider while the user is dragging it!
    if ( !m_sliding || m_outside )
        QSlider::setValue( newValue );
    else
        m_prevValue = newValue;
}

void Amarok::Slider::paintCustomSlider( QPainter *p, bool paintMoodbar )
{
    qreal percent = 0.0;
    if ( maximum() > minimum() )
        percent = ((qreal)value()) / ( maximum() - minimum() );
    QStyleOptionSlider opt;
    initStyleOption( &opt );
    if ( m_sliding ||
        ( underMouse() && sliderHandleRect( rect(), percent ).contains( mapFromGlobal(QCursor::pos()) ) ) )
    {
        opt.activeSubControls |= QStyle::SC_SliderHandle;
    }
    The::svgHandler()->paintCustomSlider( p, &opt, percent, paintMoodbar );
}


//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS VolumeSlider
//////////////////////////////////////////////////////////////////////////////////////////

Amarok::VolumeSlider::VolumeSlider( uint max, QWidget *parent, bool customStyle )
    : Amarok::Slider( customStyle ? Qt::Horizontal : Qt::Vertical, max, parent )
{
    m_usingCustomStyle = customStyle;
    setFocusPolicy( Qt::NoFocus );
    setInvertedAppearance( false );
    setInvertedControls( false );
}

void
Amarok::VolumeSlider::mousePressEvent( QMouseEvent *e )
{
    if( e->button() != Qt::RightButton )
    {
        Amarok::Slider::mousePressEvent( e );
        slideEvent( e );
    }
}

void
Amarok::VolumeSlider::contextMenuEvent( QContextMenuEvent *e )
{
    QMenu menu;
    menu.setTitle(   i18n( "Volume" ) );
    menu.addAction(  i18n(   "100%" ) )->setData( 100 );
    menu.addAction(  i18n(    "80%" ) )->setData(  80 );
    menu.addAction(  i18n(    "60%" ) )->setData(  60 );
    menu.addAction(  i18n(    "40%" ) )->setData(  40 );
    menu.addAction(  i18n(    "20%" ) )->setData(  20 );
    menu.addAction(  i18n(     "0%" ) )->setData(   0 );

    /*
    // TODO: Phonon
    menu.addSeparator();
    menu.addAction( QIcon::fromTheme( "view-media-equalizer-amarok" ), i18n( "&Equalizer" ), qApp, &QCoreApplication::slotConfigEqualizer()) )->setData( -1 );
    */

    QAction* a = menu.exec( mapToGlobal( e->pos() ) );
    if( a )
    {
        const int n = a->data().toInt();
        if( n >= 0 )
        {
            QSlider::setValue( n );
            Q_EMIT sliderReleased( n );
        }
    }
}

void
Amarok::VolumeSlider::wheelEvent( QWheelEvent *e )
{
    const uint step = e->angleDelta().y() / Amarok::VOLUME_SENSITIVITY; //FIXME: check if .x() must be used
    QSlider::setValue( QSlider::value() + step );

    Q_EMIT sliderReleased( value() );
}

void
Amarok::VolumeSlider::paintEvent( QPaintEvent *event )
{
    if( m_usingCustomStyle )
    {
        QPainter p( this );
        paintCustomSlider( &p );
        p.end();
        return;
    }

    QSlider::paintEvent( event );
}


//////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// TIMESLIDER ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

Amarok::TimeSlider::TimeSlider( QWidget *parent )
    : Amarok::Slider( Qt::Horizontal, 0, parent )
    , m_triangles()
    , m_knobX( 0.0 )
{
    m_usingCustomStyle = true;
    setFocusPolicy( Qt::NoFocus );
}

void
Amarok::TimeSlider::setSliderValue( int value )
{
    Amarok::Slider::setValue( value );
}

void
Amarok::TimeSlider::paintEvent( QPaintEvent *pe )
{
    QPainter p( this );
    p.setClipRegion( pe->region() );
    paintCustomSlider( &p, AmarokConfig::showMoodbarInSlider() );
    p.end();

}

void Amarok::TimeSlider::resizeEvent(QResizeEvent * event)
{
    Amarok::Slider::resizeEvent( event );
    The::amarokUrlHandler()->updateTimecodes();
}

void Amarok::TimeSlider::sliderChange( SliderChange change )
{
    if ( change == SliderValueChange || change == SliderRangeChange )
    {
        int oldKnobX = m_knobX;
        qreal percent = 0.0;
        if ( maximum() > minimum() )
            percent = ((qreal)value()) / ( maximum() - minimum() );
        QRect knob = sliderHandleRect( rect(), percent );
        m_knobX = knob.x();

        if (oldKnobX < m_knobX)
            update( oldKnobX, knob.y(), knob.right() + 1 - oldKnobX, knob.height() );
        else if (oldKnobX > m_knobX)
            update( m_knobX, knob.y(), oldKnobX + knob.width(), knob.height() );
    }
    else
        Amarok::Slider::sliderChange( change ); // calls update()
}

void Amarok::TimeSlider::drawTriangle( const QString& name, int milliSeconds, bool showPopup )
{
    DEBUG_BLOCK
    int sliderHeight = height() - ( s_sliderInsertY * 2 );
    int sliderLeftWidth = sliderHeight / 3;

    // This mess converts the # of seconds into the pixel width value where the triangle should be drawn
    int x_pos = ( ( ( double ) milliSeconds - ( double ) minimum() ) / ( maximum() - minimum() ) ) * ( width() - ( sliderLeftWidth + sliderLeftWidth + s_sliderInsertX * 2 ) );
    debug() << "drawing triangle at " << x_pos;
    BookmarkTriangle * tri = new BookmarkTriangle( this, milliSeconds, name, width(), showPopup );
    connect( tri, &BookmarkTriangle::clicked, this, &TimeSlider::slotTriangleClicked );
    connect( tri, &BookmarkTriangle::focused, this, &TimeSlider::slotTriangleFocused );
    m_triangles << tri;
    tri->setGeometry( x_pos + 6 /* to center the point */, 1 /*y*/, 11, 11 ); // 6 = hard coded border width
    tri->show();
}

void Amarok::TimeSlider::slotTriangleClicked( int seconds )
{
    Q_EMIT sliderReleased( seconds );
}

void Amarok::TimeSlider::slotTriangleFocused( int seconds )
{
    QList<BookmarkTriangle *>::iterator i;
    for( i = m_triangles.begin(); i != m_triangles.end(); ++i ){
         if( (*i)->getTimeValue() != seconds )
             (*i)->hidePopup();
    }
}

void Amarok::TimeSlider::clearTriangles()
{
    QList<BookmarkTriangle *>::iterator i;
    for( i = m_triangles.begin(); i != m_triangles.end(); ++i ){
      (*i)->deleteLater();
    }
    m_triangles.clear();
}

void Amarok::TimeSlider::mousePressEvent( QMouseEvent *event )
{
    if( !The::engineController()->isSeekable() )
        return; // Eat the event,, it's not possible to seek
    Amarok::Slider::mousePressEvent( event );
}

bool Amarok::TimeSlider::event( QEvent * event )
{
    if( event->type() == QEvent::ToolTip )
    {
        // Make a QHelpEvent out of this
        QHelpEvent * helpEvent = dynamic_cast<QHelpEvent *>( event );
        if( helpEvent )
        {

            //figure out "percentage" of the track length that the mouse is hovering over the slider
            qreal percentage = (qreal) helpEvent->x() / (qreal) width();
            long trackLength = The::engineController()->trackLength();
            int trackPosition = trackLength * percentage;

            // Update tooltip to show the track position under the cursor
            setToolTip( i18nc( "Tooltip shown when the mouse is over the progress slider, representing the position in the currently playing track that Amarok will seek to if you click the mouse. Keep it concise.", "Jump to: %1", Meta::msToPrettyTime( trackPosition ) ) );
        }
    }

    return QWidget::event( event );
}


