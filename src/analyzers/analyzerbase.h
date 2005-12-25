// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright:  See COPYING file that comes with this distribution

#ifndef ANALYZERBASE_H
#define ANALYZERBASE_H


#include <config.h>  //HAVE_QGLWIDGET

#ifdef __FreeBSD__
#include <sys/types.h>
#endif

#include "fht.h"     //stack allocated and convenience
#include <qpixmap.h> //stack allocated and convenience
#include <qtimer.h>  //stack allocated
#include <qwidget.h> //baseclass
#include <vector>    //included for convenience

#ifdef HAVE_QGLWIDGET
#include <qgl.h>     //baseclass
#ifdef Q_WS_MACX
#include <OpenGL/gl.h>   //included for convenience
#include <OpenGL/glu.h>  //included for convenience
#else
#include <GL/gl.h>   //included for convenience
#include <GL/glu.h>  //included for convenience
#endif
#else
//this is a workaround for compile problems due to moc
#define QGLWidget QWidget
#endif

class QEvent;
class QPaintEvent;
class QResizeEvent;


namespace Analyzer {

typedef std::vector<float> Scope;

template<class W> class Base : public W
{
public:
    uint timeout() const { return m_timeout; }

protected:
    Base( QWidget*, uint, uint = 7 );
    ~Base() { delete m_fht; }

    void drawFrame();
    int  resizeExponent( int );
    int  resizeForBands( int );
    virtual void transform( Scope& );
    virtual void analyze( const Scope& ) = 0;
    virtual void paused();
    virtual void demo();

    void changeTimeout( uint newTimeout )
    {
        m_timer.changeInterval( newTimeout );
        m_timeout = newTimeout;
    }

private:
    bool event( QEvent* );

protected:
    QTimer m_timer;
    uint   m_timeout;
    FHT    *m_fht;
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
    Base2D( QWidget*, uint timeout, uint scopeSize = 7 );

    virtual void init() {}

    QPixmap *background() { return &m_background; }
    QPixmap     *canvas() { return &m_canvas; }
    void    eraseCanvas() { bitBlt( canvas(), 0, 0, background() ); }

    void paintEvent( QPaintEvent* ) { if( !m_canvas.isNull() ) bitBlt( this, 0, 0, canvas() ); }
    void resizeEvent( QResizeEvent* );
    void paletteChange( const class QPalette& );

    void polish();

private:
    QPixmap m_background;
    QPixmap m_canvas;
};



//This mess is because moc generates an entry for this class despite the #if block
//1. the Q_OBJECT macro must be exposed
//2. we have to define the class
//3. we have to declare a ctor (to satisfy the inheritance)
//4. the slot must also by visible (!)
//TODO find out how to stop moc generating a metaobject for this class
class Base3D : public Base<QGLWidget>
{
Q_OBJECT
#ifdef HAVE_QGLWIDGET
protected:
    Base3D( QWidget*, uint, uint = 7 );
private slots:
    void draw() { drawFrame(); }
#else
protected:
    Base3D( QWidget *w, uint i1, uint i2 ) : Base<QGLWidget>( w, i1, i2 ) {}
private slots:
    void draw() {}
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
    static QWidget* createPlaylistAnalyzer( QWidget *);
};


void interpolate( const Scope&, Scope& );
void initSin( Scope&, const uint = 6000 );

} //END namespace Analyzer

using Analyzer::Scope;

#endif
