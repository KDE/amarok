// Author:    Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef AMKVIS_H
#define AMKVIS_H

#include <qwidget.h>
#include "vis.h" //TODO we need to install so you include "amarok/vis.h"

class QTimerEvent;

class MyVis : public QWidget, public amaroK::Vis
{
public:
    MyVis();

    void timerEvent( QTimerEvent* );
};

#endif
