//
// C++ Implementation: MainToolbar
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "MainToolbar.h"

#include "debug.h"

#include <KStandardDirs>   

#include <QPainter>

MainToolbar::MainToolbar( QWidget * parent )
 : KHBox( parent )
{

    m_svgRenderer = new QSvgRenderer( KStandardDirs::locate( "data","amarok/images/toolbar-background.svg" ));
    if ( ! m_svgRenderer->isValid() )
        debug() << "svg is kaputski";
}


MainToolbar::~MainToolbar()
{
}

void MainToolbar::paintEvent(QPaintEvent *)
{

    int middle = contentsRect().width() / 2;
    QRect controlRect( middle - 125, 0, 250, 50 );

    QPainter pt( this );
    m_svgRenderer->render( &pt, "toolbarbackground",  contentsRect() );
    m_svgRenderer->render( &pt, "buttonbar",  controlRect );

}


