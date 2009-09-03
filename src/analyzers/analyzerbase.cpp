/****************************************************************************************
 * Copyright (c) 2003 Max Howell <max.howell@methylblue.com>                            *
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

#include "analyzerbase.h"

#include <cmath>        //interpolate()
#include "EngineController.h"
#include <QEvent>     //event()
//Added by qt3to4:
#include <QResizeEvent>


// INSTRUCTIONS Base2D
// 1. do anything that depends on height() in init(), Base2D will call it before you are shown
// 2. otherwise you can use the constructor to initialise things
// 3. reimplement analyze(), and paint to canvas(), Base2D will update the widget when you return control to it
// 4. if you want to manipulate the scope, reimplement transform()
// 5. for convenience <vector> <qpixmap.h> <qwdiget.h> are pre-included
// TODO make an INSTRUCTIONS file
//can't mod scope in analyze you have to use transform


//TODO for 2D use setErasePixmap Qt function insetead of m_background

// make the linker happy only for gcc < 4.0
#if !( __GNUC__ > 4 || ( __GNUC__ == 4 && __GNUC_MINOR__ >= 0 ) )
template class Analyzer::Base<QWidget>;
#endif


template<class W>
Analyzer::Base<W>::Base( QWidget *parent, uint timeout, uint scopeSize )
        : W( parent )
        , m_timeout( timeout )
        , m_fht( new FHT(scopeSize) )
{}

template<class W> bool
Analyzer::Base<W>::event( QEvent *e )
{
    switch( e->type() ) {

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

    //NOTE resizing here is redundant as FHT routines only calculate FHT::size() values
    //scope.resize( m_fht->size() );

    float *front = static_cast<float*>( &scope.front() );

    float* f = new float[ m_fht->size() ];
    m_fht->copy( &f[0], front );
    m_fht->logSpectrum( front, &f[0] );
    m_fht->scale( front, 1.0 / 20 );

    scope.resize( m_fht->size() / 2 ); //second half of values are rubbish
    delete [] f;
}

template<class W> void
Analyzer::Base<W>::drawFrame()
{
#if 0
    EngineBase *engine = EngineController::engine();

    switch( engine->state() )
    {
    case Engine::Playing:
    {
        const Engine::Scope &thescope = engine->scope();
        static Analyzer::Scope scope( 512 );
        int i = 0;

        // convert to mono here - our built in analyzers need mono, but we the engines provide interleaved pcm
        for( uint x = 0; (int)x < m_fht->size(); ++x )
        {
           scope[x] = double(thescope[i] + thescope[i+1]) / (2*(1<<15));
           i += 2;
        }

        transform( scope );
        analyze( scope );

        scope.resize( m_fht->size() );

        break;
    }
    case Engine::Paused:
        paused();
        break;

    default:
        demo();
    }
#endif
}

template<class W> int
Analyzer::Base<W>::resizeExponent( int exp )
{
    if ( exp < 3 )
        exp = 3;
    else if ( exp > 9 )
        exp = 9;

    if ( exp != m_fht->sizeExp() ) {
        delete m_fht;
        m_fht = new FHT( exp );
    }
    return exp;
}

template<class W> int
Analyzer::Base<W>::resizeForBands( int bands )
{
    int exp;
    if ( bands <= 8 )
        exp = 4;
    else if ( bands <= 16 )
        exp = 5;
    else if ( bands <= 32 )
        exp = 6;
    else if ( bands <= 64 )
        exp = 7;
    else if ( bands <= 128 )
        exp = 8;
    else
        exp = 9;

    resizeExponent( exp );
    return m_fht->size() / 2;
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
    //TODO is there much point in this anymore?

    //we use polish for initialzing (instead of ctor)
    //because we need to know the widget's final size
    QWidget::ensurePolished();

    init(); //virtual
}

void
Analyzer::Base2D::resizeEvent( QResizeEvent *e )
{
    m_background = QPixmap( size() );

    QWidget::resizeEvent( e );
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
        const double error = pos - std::floor( pos );
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
