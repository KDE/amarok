/***************************************************************************
                               analyzerbase.h
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

#ifndef ANALYZERBASE_H
#define ANALYZERBASE_H

#ifdef __FreeBSD__
    #include <sys/types.h>
#endif

#undef  DRAW_GRID  //disable the grid

#include <config.h>  //HAVE_QGLWIDGET
#include "../fht.h"  //FIXME
#include <qgl.h>     //baseclass
#include <qpixmap.h> //stack allocated
#include <qtimer.h>  //stack allocated
#include <qwidget.h> //baseclass
#include <vector>    //included for convenience

#ifdef HAVE_QGLWIDGET
#include <GL/gl.h>  //included for convenience
#include <GL/glu.h> //included for convenience
#endif

class QEvent;
class QPaintEvent;


namespace Analyzer {

typedef std::vector<float> Scope;

//TODO consider making an m_scope
//TODO consider manipulating Scopes by reference
//TODO init() doesn't get called for GLWidgets, move init() to Base2D or be clever
//TODO offer a vector full of zero instead of 0 as Scope * (? worth it considering interpolation etc.)
//perhaps you should try to enforce modification of the scope in modifyScope by passing a const reference to the scope to drawAnalyzer?

template<class W> class Base : public W
{
public:
    uint timeout() const { return m_timeout; }
    uint height()  const { return m_height; }

protected:
    Base( QWidget*, uint, uint = 7 );

    void drawFrame();

    virtual void drawAnalyzer( Scope* ) = 0;
    virtual void modifyScope( float* );

private:
    bool event( QEvent* );

protected:
    QTimer m_timer;
    uint   m_timeout;
    uint   m_height;
    FHT    m_fht;
};


class Base2D : public Base<QWidget>
{
Q_OBJECT
public:
    const QPixmap *background() const { return &m_background; }
    const QPixmap *canvas()     const { return &m_canvas; }

private slots:
    void draw() { drawFrame(); bitBlt( this, 0, 0, canvas() ); }

protected:
    Base2D( QWidget*, uint, uint = 7 );

    virtual void init() {}

    QPixmap  *canvas() { return &m_canvas; }
    void eraseCanvas() { bitBlt( canvas(), 0, 0, background() ); }

private:
    void polish();
    void paintEvent( QPaintEvent* ) { if( !m_canvas.isNull() ) bitBlt( this, 0, 0, canvas() ); }

    QPixmap m_background;
    QPixmap m_canvas;
};


class Base3D : public Base<QGLWidget>
{
#ifdef HAVE_QGLWIDGET
Q_OBJECT
protected:
    Base3D( QWidget*, uint, uint = 7 );
private slots:
    void draw() { drawFrame(); }
#endif
};


class Factory
{
    //Currently this is a rather small class, its only purpose
    //to ensure that making changes to analyzers will not require
    //rebuilding the world!

    //eventually it would be better to make analyzers pluggable
    //but I can't be arsed, nor can I see much reason to do so
    //yet!
public:
    static QWidget* createAnalyzer( QWidget* );
};


void interpolate( const Scope*, Scope& );
void initSin( Scope&, const uint = 6000 );

} //END namespace Analyzer

#endif
