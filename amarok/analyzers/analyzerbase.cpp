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

#include <math.h>   //interpolate()
#include <qevent.h> //event()

#ifdef DRAW_GRID
#include <qpainter.h>
#include <qpen.h>
#endif


// INSTRUCTIONS Base2D
// 1. do anything that depends on height() in init(), Base2D will call it before you are shown
// 2. otherwise you can use the constructor to initialise things
// 3. reimplement drawAnalyzer(), and paint to canvas(), Base2D will update the widget when you return control to it
// 4. if you want to manipulate the scope, reimplement modifyScope()
// 5. for convenience <vector> <qpixmap.h> <qwdiget.h> are pre-included
// TODO make an INSTRUCTIONS file


template<class W>
Analyzer::Base<W>::Base( QWidget *parent, uint timeout, uint size )
  : W( parent )
  , m_timeout( timeout )
  , m_height( 0 )
  , m_fht( size )
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

#include "../playerapp.h" //FIXME
#include "../engine/enginebase.h" //FIXME

template<class W> void
Analyzer::Base<W>::drawFrame()
{
    EngineBase *engine = pApp->m_pEngine;

    if ( engine->state() == EngineBase::Playing )
    {
        Scope *scope = engine->scope();
        scope->resize( scope->size() / 2 );

        float *front = static_cast<float*>( &scope->front() );
        if( front )
        {
            modifyScope( front );
            drawAnalyzer( scope );
        }

        delete scope;
    }
    else
    {
        static int t = 201;

        if ( t > 999 ) t = 1; //0 = wasted calculations
        if ( t < 201 )
        {
            const double dt = double(t) / 200 ;
            Scope s( 32 );
            for( uint i = 0; i < s.size(); ++i )
                s[i] = dt * (sin( M_PI + (i * M_PI) / s.size() ) + 1.0);
            drawAnalyzer( &s );
        }
        else
            drawAnalyzer( 0 );

        ++t;
    }
}

template<class W> void
Analyzer::Base<W>::modifyScope( float *front ) //virtual
{
    float *f = new float[ m_fht.size() ];
    m_fht.copy( f, front );
    m_fht.logSpectrum( front, f );
    m_fht.scale( front, 1.0 / 20 );
    delete[] f;
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
Analyzer::interpolate( const Scope *inVec, Scope &outVec ) //static
{
    double pos = 0.0;
    double step = static_cast<double>(inVec->size()) / outVec.size();

    for ( uint i = 0; i < outVec.size(); ++i, pos += step )
    {
        double error = pos - floor( pos );
        unsigned long offset = static_cast<unsigned long>( pos );

        unsigned long indexLeft = offset + 0;

        if ( indexLeft >= inVec->size() )
            indexLeft = inVec->size() - 1;

        unsigned long indexRight = offset + 1;

        if ( indexRight >= inVec->size() )
            indexRight = inVec->size() - 1;

        outVec[i] = (*inVec)[indexLeft ] * ( 1.0 - error ) +
                    (*inVec)[indexRight] * error;
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
