/****************************************************************************************
 * Copyright (c) 2003 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2009 Martin Sandsmark <martin.sandsmark@kde.org>                       *
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

#include "AnalyzerBase.h"

#include "EngineController.h"

#include <cmath> // interpolate()

#include <QEvent> // event()
#include <QPainter>

#include <phonon/audiodataoutput.h>


// INSTRUCTIONS Base2D
// 1. do anything that depends on height() in init(), Base2D will call it before you are shown
// 2. otherwise you can use the constructor to initialise things
// 3. reimplement analyze(), and paint to canvas(), Base2D will update the widget when you return control to it
// 4. if you want to manipulate the scope, reimplement transform()
// 5. for convenience <vector> <qpixmap.h> <qwdiget.h> are pre-included
// TODO make an INSTRUCTIONS file
//can't mod scope in analyze you have to use transform


Analyzer::Base::Base( QWidget *parent, uint scopeSize )
    : QWidget( parent )
    , m_fht( new FHT( scopeSize ) )
{
    connect( EngineController::instance(), SIGNAL( audioDataReady( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ),
             this,   SLOT( drawFrame( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ) );
}

void
Analyzer::Base::transform( QVector<float> &scope ) //virtual
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

void
Analyzer::Base::drawFrame( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > &thescope )
{
    if( thescope.isEmpty() )
        return;

    static QVector<float> scope( 512 );
    int i = 0;

    for( uint x = 0; ( int )x < m_fht->size(); ++x )
    {
        if( thescope.size() == 1 )  // Mono
        {
            scope[x] = double( thescope[Phonon::AudioDataOutput::LeftChannel][x] );
        }
        else     // Anything > Mono is treated as Stereo
        {
            scope[x] = double( thescope[Phonon::AudioDataOutput::LeftChannel][x]
                               + thescope[Phonon::AudioDataOutput::RightChannel][x] )
                       / ( 2 * ( 1 << 15 ) ); // Average between the channels
        }
        i += 2;
    }

    transform( scope );
    analyze( scope );

    scope.resize( m_fht->size() );

    update();
}

int
Analyzer::Base::resizeExponent( int exp )
{
    if( exp < 3 )
        exp = 3;
    else if( exp > 9 )
        exp = 9;

    if( exp != m_fht->sizeExp() )
    {
        delete m_fht;
        m_fht = new FHT( exp );
    }
    return exp;
}

int
Analyzer::Base::resizeForBands( int bands )
{
    int exp;
    if( bands <= 8 )
        exp = 4;
    else if( bands <= 16 )
        exp = 5;
    else if( bands <= 32 )
        exp = 6;
    else if( bands <= 64 )
        exp = 7;
    else if( bands <= 128 )
        exp = 8;
    else
        exp = 9;

    resizeExponent( exp );
    return m_fht->size() / 2;
}

void
Analyzer::Base::paused() //virtual
{}

void
Analyzer::Base::demo() //virtual
{
    static int t = 201; //FIXME make static to namespace perhaps
//    qDebug() << Q_FUNC_INFO << t;

    if( t > 300 ) t = 1; //0 = wasted calculations
    if( t < 201 )
    {
        QVector<float> s( 512 );

        const double dt = double( t ) / 200;
        for( int i = 0; i < s.size(); ++i )
            s[i] = dt * ( sin( M_PI + ( i * M_PI ) / s.size() ) + 1.0 );

        analyze( s );
    }
    else analyze( QVector<float>( 1, 0 ) );

    ++t;
}



Analyzer::Base2D::Base2D( QWidget *parent, uint scopeSize )
    : Base( parent, scopeSize )
{
    connect( EngineController::instance(), SIGNAL( playbackStateChanged() ), this, SLOT( playbackStateChanged() ) );

    m_timer.setInterval( 34 );
    connect( &m_timer, SIGNAL( timeout() ), this, SLOT( demo() ) );

    enableDemo( !EngineController::instance()->isPlaying() );
    QTimer::singleShot( 0, this, SLOT( init() ) ); // needs to know the size
}

void Analyzer::Base2D::resizeEvent( QResizeEvent *e )
{
    QWidget::resizeEvent( e );

    m_canvas = QPixmap( size() );
    m_canvas.fill( Qt::transparent );
}

void Analyzer::Base2D::paintEvent( QPaintEvent* )
{
    if( m_canvas.isNull() )
        return;

    QPainter painter( this );
    painter.drawPixmap( rect(), m_canvas );
}

void Analyzer::Base2D::playbackStateChanged()
{
    enableDemo( !EngineController::instance()->isPlaying() );
}

void
Analyzer::interpolate( const QVector<float> &inVec, QVector<float> &outVec ) //static
{
    double pos = 0.0;
    const double step = ( double )inVec.size() / outVec.size();

    for( int i = 0; i < outVec.size(); ++i, pos += step )
    {
        const double error = pos - std::floor( pos );
        const unsigned long offset = ( unsigned long )pos;

        long indexLeft = offset + 0;

        if( indexLeft >= inVec.size() )
            indexLeft = inVec.size() - 1;

        long indexRight = offset + 1;

        if( indexRight >= inVec.size() )
            indexRight = inVec.size() - 1;

        outVec[i] = inVec[indexLeft ] * ( 1.0 - error ) +
                    inVec[indexRight] * error;
    }
}

void
Analyzer::initSin( QVector<float> &v, const uint size ) //static
{
    double step = ( M_PI * 2 ) / size;
    double radian = 0;

    for( uint i = 0; i < size; i++ )
    {
        v.push_back( sin( radian ) );
        radian += step;
    }
}
