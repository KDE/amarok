// Author:    Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#include "barscope.h"

#include <qapplication.h>
#include <qpainter.h>
#include <qwidget.h>


int
main( int argc, char **argv )
{
    QApplication a( argc, argv );
    MyVis v;

    v.show();
    a.setMainWidget( &v );

    return a.exec();
}

MyVis::MyVis() : QWidget(), amaroK::Vis()
{
    startTimer( 100 );
}

void
MyVis::timerEvent( QTimerEvent* )
{
    QPainter p( this );

    p.setPen( Qt::red );
    p.fillRect( 10, 10, width() - 20, height() - 20, Qt::blue );
}
