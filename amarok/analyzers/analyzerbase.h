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

#define SINVEC_SIZE 6000
#undef  DRAW_GRID  //disable the grid

class QWidget;
//namespace std { template<class T> class vector; }
#include <vector> //couldn't get the predeclaration right :(

class AnalyzerBase
{
public:
    virtual ~AnalyzerBase();

    virtual void drawAnalyzer( std::vector<float>* ) = 0;
    uint timeout() const { return m_timeout; }

protected:
    AnalyzerBase( uint );

    void interpolate( std::vector<float>*, std::vector<float>& ) const;
    void initSin( std::vector<float> & ) const;

    virtual void init() = 0;

private:
    uint m_timeout;

public:

    class AnalyzerFactory
    {
        //Currently this is a rather small class, its only purpose
        //to ensure that making changes to analyzers will not require
        //rebuilding the world!

        //eventually it would be better to make analyzers pluggable
        //but I can't be arsed, nor can I see much reason to do so
        //yet!
    public:
        static AnalyzerBase *createAnalyzer( QWidget* );
    };
};



#include <qwidget.h>  //baseclass
#include <qpixmap.h>  //stack allocated

/**
 *@author PiggZ
 */

class AnalyzerBase2d : public QWidget, public AnalyzerBase
{
    Q_OBJECT

    public:
        //this is called often in drawAnalyser implementations
        //so you felt you had to shorten the workload by re-implementing it
        //but! don't forget to set it to the new value for height when
        //we start allowing the main Widget to be resized
        uint height() const { return m_height; }
        const QPixmap *grid() const { return &m_background; } //DEPRECATE
        const QPixmap *background() const { return &m_background; }

    protected:
        AnalyzerBase2d( uint, QWidget* =0, const char* =0 );

    private:
        void polish();

        uint m_height;
        QPixmap m_background;
};



#include <config.h>
#ifdef HAVE_QGLWIDGET

#include <qgl.h>    //basclass
#include <GL/gl.h>  //include for convenience
#include <GL/glu.h> //include for convenience

/**
 *@author PiggZ
 */

class AnalyzerBase3d : public QGLWidget, public AnalyzerBase
{
    Q_OBJECT

    protected:
        AnalyzerBase3d( uint, QWidget* =0, const char* =0 );
};

#endif
#endif
