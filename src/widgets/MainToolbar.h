//
// C++ Interface: MainToolbar
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MAINTOOLBAR_H
#define MAINTOOLBAR_H

#include <KHBox>

#include <QSvgRenderer>

/**
A KHBox based toolbar with a nice svg background

	@author 
*/
class MainToolbar : public KHBox
{
public:
    MainToolbar( QWidget * parent );

    ~MainToolbar();

protected:
      virtual void paintEvent(QPaintEvent *);

private:

    QSvgRenderer * m_svgRenderer;
};

#endif
