/***************************************************************************
                          viswidget.cpp  -  description
                             -------------------
    begin                : Die Jan 7 2003
    copyright            : (C) 2003 by Max Howell
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

#include "analyzerbase.h"
#include "enginebase.h" //engine->state()
#include <math.h>       //interpolate()
#include "playerapp.h"  //m_pEngine
#include <qevent.h>     //event()

#ifdef DRAW_GRID
#include <qpainter.h>
#include <qpen.h>
#endif


// INSTRUCTIONS Base2D
// 1. do anything that depends on height() in init(), Base2D will call it before you are shown
// 2. otherwise you can use the constructor to initialise things
// 3. reimplement analyze(), and paint to canvas(), Base2D will update the widget when you return control to it
// 4. if you want to manipulate the scope, reimplement transform()
// 5. for convenience <vector> <qpixmap.h> <qwdiget.h> are pre-included
// TODO make an INSTRUCTIONS file
//can't mod scope in analyze you have to use transform

template<class W>
Analyzer::Base<W>::Base( QWidget *parent, uint timeout, uint scopeSize )
  : W( parent )
  , m_timeout( timeout )
  , m_height( 0 )
  , m_fht( scopeSize )
{}

template<class W> bool
Analyzer::Base<W>::event( QEvent *e )
{
    switch( e->type() ) {
/*    case QEvent::Paint:
        if( !canvas()->isNull() )
            bitBlt( this, 0, 0, canvas() );
        return true; //no propagate event*/
    case QEvent::Hide:
        m_timer.stop();
        break;
    case QEvent::Show:
        m_timer.start( timeout() );
        break;
    default:
        break;
    }

    return QWidget::event( e );
}

template<class W> void
Analyzer::Base<W>::transform( Scope &scope ) //virtual
{
    //this is a standard transformation that should give
    //an FFT scope that has bands for pretty analyzers

    scope.resize( scope.size() / 2 ); //why do we do this?

    float *front = static_cast<float*>( &scope.front() );

    float f[ m_fht.size() ];
    m_fht.copy( &f[0], front );
    m_fht.logSpectrum( front, &f[0] );
    m_fht.scale( front, 1.0 / 20 );
}

template<class W> void
Analyzer::Base<W>::drawFrame()
{
    EngineBase *engine = pApp->m_pEngine;

    switch( engine->state() )
    {
    case EngineBase::Playing:
    {
        Scope *scope = engine->scope();

        if( !scope->empty() )
        {
            transform( *scope );
            analyze( *scope );
        }

        delete scope;

        break;
    }
    case EngineBase::Paused:
        paused();
        break;

    default:
        demo();
    }
}

template<class W> void
Analyzer::Base<W>::paused() //virtual
{}

template<class W> void
Analyzer::Base<W>::demo() //virtual
{
    static int t = 201; //FIXME make static to namespace perhaps

    if( t > 999 ) t = 1; //0 = wasted calculations
    if( t < 201 )
    {
        Scope s( 32 );

        const double dt = double(t) / 200;
        for( uint i = 0; i < s.size(); ++i )
            s[i] = dt * (sin( M_PI + (i * M_PI) / s.size() ) + 1.0);

        analyze( s );
    }
    else analyze( Scope( 32, 0 ) );

    ++t;
}



Analyzer::Base2D::Base2D( QWidget *parent, uint timeout, uint scopeSize )
   : Base<QWidget>( parent, timeout, scopeSize )
{
    connect( &m_timer, SIGNAL( timeout() ), SLOT( draw() ) );
}

void
Analyzer::Base2D::polish()
{
    //we use polish for initialzing (instead of ctor)
    //because we need to know the widget's final size
    QWidget::polish();

    m_height = QWidget::height();
    m_background.resize( size() );
    m_canvas.resize( size() );

    #ifdef DRAW_GRID
    QPainter p( &m_background );
    p.setPen( QColor( 0x20, 0x20, 0x50 ) );

    for( uint x = 0, w = m_background.width(), h = m_background.height()-1;
         x < w; x += 3 ) p.drawLine( x, 0, x, h );
    for( uint y = 0, w = m_background.width()-1 , h = m_background.height();
         y < h; y += 3 ) p.drawLine( 0, y, w, y );
    #else
    m_background.fill( backgroundColor() );
    #endif

    eraseCanvas(); //this is necessary

    init(); //virtual
}



#ifdef HAVE_QGLWIDGET
Analyzer::Base3D::Base3D( QWidget *parent, uint timeout, uint scopeSize )
   : Base<QGLWidget>( parent, timeout, scopeSize )
{
    connect( &m_timer, SIGNAL( timeout() ), SLOT( draw() ) );
}
#endif


void
Analyzer::interpolate( const Scope &inVec, Scope &outVec ) //static
{
    double pos = 0.0;
    const double step = (double)inVec.size() / outVec.size();

    for ( uint i = 0; i < outVec.size(); ++i, pos += step )
    {
        const double error = pos - floor( pos );
        const unsigned long offset = (unsigned long)pos;

        unsigned long indexLeft = offset + 0;

        if ( indexLeft >= inVec.size() )
            indexLeft = inVec.size() - 1;

        unsigned long indexRight = offset + 1;

        if ( indexRight >= inVec.size() )
            indexRight = inVec.size() - 1;

        outVec[i] = inVec[indexLeft ] * ( 1.0 - error ) +
                    inVec[indexRight] * error;
    }
}

void
Analyzer::initSin( Scope &v, const uint size ) //static
{
    double step = ( M_PI * 2 ) / size;
    double radian = 0;

    for ( uint i = 0; i < size; i++ )
    {
        v.push_back( sin( radian ) );
        radian += step;
    }
}

#include "analyzerbase.moc"
