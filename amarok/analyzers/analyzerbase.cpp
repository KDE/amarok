/***************************************************************************
                          viswidget.cpp  -  description
                             -------------------
    begin                : Die Jan 7 2003
    copyright            : (C) 2003 by Mark Kretschmann
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

#include "analyzerbase.h"

#include <math.h>
#include <vector>

#include <qimage.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>


// INSTRUCTIONS
// 1. inherit AnalyzerBase( first parameter to AnalyzerBase is the frequency (in milliseconds) that drawAnalyser will be called)
// 2. do anything that depends on height() in init()
// 3. otherwise you can use the constructor
// 4. blt to this at the end of your implementation of drawAnalyser()


AnalyzerBase::AnalyzerBase( uint timeout, QWidget *parent, const char *name ) :
   QFrame( parent, name ),
   m_timeout( timeout )
{}

AnalyzerBase::~AnalyzerBase()
{}


// METHODS =====================================================

void AnalyzerBase::polish()
{
    QWidget::polish();

    m_iVisHeight = QWidget::height();

    //we use polish for initialzing (instead of ctor), because we need to know the widget's final size
    initGrid();
    init(); //virtual
}


void AnalyzerBase::init()
{}


void AnalyzerBase::initGrid()
{
    m_grid.resize( width(), height() );
    bitBlt( &m_grid, 0, 0, parentWidget()->paletteBackgroundPixmap(), x(), y(), width(), height() );

    QPainter painterGrid( &m_grid );
    painterGrid.setPen( QPen( QColor( 0x20, 0x20, 0x50 ) ) );

    for( int x = 0, w = m_grid.width(), h = m_grid.height()-1;
         x < w;
         x += 3 )
    {
        painterGrid.drawLine( x, 0, x, h );
    }

    for( int y = 0, w = m_grid.width()-1 , h = m_grid.height();
         y < h;
         y += 3 )
    {
        painterGrid.drawLine( 0, y, w, y );
    }
}


void AnalyzerBase::initSin( std::vector<float> &v ) const
{
    double step = ( M_PI * 2 ) / SINVEC_SIZE;
    double radian = 0;

    for ( int i = 0; i < SINVEC_SIZE; i++ )
    {
        v.push_back( sin( radian ) );
        radian += step;
    }
}


void AnalyzerBase::mouseReleaseEvent( QMouseEvent * )
{
    emit clicked();
}

// FIXME got this segfault in interpolate():
// [New Thread 1024 (LWP 10507)]
// [New Thread 2049 (LWP 10509)]
// 0x420ff449 in wait4 () from /lib/libc.so.6
// #0  0x420ff449 in wait4 () from /lib/libc.so.6
// #1  0x4217bfd0 in __DTOR_END__ () from /lib/libc.so.6
// #2  0x41f24a73 in waitpid () from /lib/libpthread.so.0
// #3  0x4141d798 in KCrash::defaultCrashHandler(int) ()
//    from /opt/kde3/lib/libkdecore.so.4
// #4  0x41f2211b in pthread_sighandler () from /lib/libpthread.so.0
// #5  <signal handler called>
// #6  0x080a3c4b in AnalyzerBase::interpolate(std::vector<float, std::allocator<float> >*, std::vector<float, std::allocator<float> >&) const (this=0x8448078, 
//     oldVec=0x822c4b0, newVec=@0xbfffed48)
//     at amarok/analyzers/analyzerbase.cpp:131
// #7  0x080a1cc5 in DistortAnalyzer::drawAnalyzer(std::vector<float, std::allocator<float> >*) (this=0x8448078, s=0x822c4b0)
//     at amarok/analyzers/distortanalyzer.cpp:70
// #8  0x0806e824 in PlayerApp::slotVisTimer() (this=0xbffff360)
//     at amarok/playerapp.cpp:1214
// #9  0x0806f028 in PlayerApp::qt_invoke(int, QUObject*) (this=0xbffff360, 
//     _id=35, _o=0xbfffee40) at amarok/playerapp.moc:200
// #10 0x41810ba0 in QObject::activate_signal(QConnectionList*, QUObject*) ()
//    from /usr/lib/qt3/lib/libqt-mt.so.3
// #11 0x418109d4 in QObject::activate_signal(int) ()
//    from /usr/lib/qt3/lib/libqt-mt.so.3
// #12 0x41b2daeb in QTimer::timeout() () from /usr/lib/qt3/lib/libqt-mt.so.3
// #13 0x418320b2 in QTimer::event(QEvent*) () from /usr/lib/qt3/lib/libqt-mt.so.3
// #14 0x417b5345 in QApplication::internalNotify(QObject*, QEvent*) ()
//    from /usr/lib/qt3/lib/libqt-mt.so.3
// #15 0x417b4735 in QApplication::notify(QObject*, QEvent*) ()
//    from /usr/lib/qt3/lib/libqt-mt.so.3
// #16 0x413ae933 in KApplication::notify(QObject*, QEvent*) ()
//    from /opt/kde3/lib/libkdecore.so.4
// #17 0x417a2adb in QEventLoop::activateTimers() ()
//    from /usr/lib/qt3/lib/libqt-mt.so.3
// #18 0x4175f8e2 in QEventLoop::processEvents(unsigned) ()
//    from /usr/lib/qt3/lib/libqt-mt.so.3
// #19 0x417c8516 in QEventLoop::enterLoop() ()
//    from /usr/lib/qt3/lib/libqt-mt.so.3
// #20 0x417c83b8 in QEventLoop::exec() () from /usr/lib/qt3/lib/libqt-mt.so.3
// #21 0x417b5591 in QApplication::exec() () from /usr/lib/qt3/lib/libqt-mt.so.3
// #22 0x080672e7 in main (argc=5, argv=0xbffff614) at amarok/main.cpp:68
// #23 0x420794f2 in __libc_start_main () from /lib/libc.so.6


void AnalyzerBase::interpolate( std::vector<float> *oldVec, std::vector<float> &newVec ) const
{
    if ( oldVec->size() )
    {    
        uint newSize = newVec.size(); //vector::size() is O(1)
    
        //necessary? code bloat if not
        if( newSize == oldVec->size() ) { newVec = *oldVec; return; }
    
        double pos = 0.0;
        double step = static_cast<double>( oldVec->size() ) / newSize;
    
        for ( uint i = 0; i < newSize; ++i, pos += step )
        {
            double error = pos - floor( pos );
            ulong offset = static_cast<unsigned long>( pos );
    
            ulong indexLeft = offset + 0;
    
            if ( indexLeft >= oldVec->size() )
                indexLeft = oldVec->size() - 1;
    
            ulong indexRight = offset + 1;
    
            if ( indexRight >= oldVec->size() )
                indexRight = oldVec->size() - 1;
    
            newVec[i] = (*oldVec)[indexLeft] * ( 1.0 - error ) +
                        (*oldVec)[indexRight] * error;
        }
    }
}

#include "analyzerbase.moc"
