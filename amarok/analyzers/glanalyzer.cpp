/***************************************************************************
                          gloscope.cpp  -  description
                             -------------------
    begin                : Jan 17 2004
    copyright            : (C) 2004 by Adam Pigg
    email                : adam@piggz.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "glanalyzer.h"

#include <math.h>
#include <vector>

#include <kdebug.h>
#include <kstandarddirs.h>

GLAnalyzer::GLAnalyzer( QWidget *parent, const char *name ):
AnalyzerBase3d(30, parent, name)
{
}

GLAnalyzer::~GLAnalyzer()
{
}

// METHODS =====================================================

void GLAnalyzer::init()
{
  x = y = -10.0f;
}

// --------------------------------------------------------------------------------

void GLAnalyzer::drawAnalyzer( std::vector<float> *s )
{  
  if (s)
  {
    interpolate(s); //if no s then we are paused/stopped
    updateGL();
  }
}

void GLAnalyzer::initializeGL()
{
	init();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);// Set clear color to black
}

void GLAnalyzer::resizeGL( int w, int h )
{
    m_bands.resize(uint(w/3) , 0.0f);
    m_oldy.resize(uint(w/3), -10.0f);
    glViewport( 0, 0, (GLint)w, (GLint)h );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho(-15.0f, 15.0f, -10.0f, 10.0f, -50.0f, 100.0f);
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
}
	
void GLAnalyzer::drawScope()
{
	//kdDebug() << "GLAnalyzer::drawScope()" << endl;
	swapBuffers();
	glColor3f(0.5f, 0.625f, 1.0f);// Set color to white
	glRotatef(0.5f, 0.2f, 1.0f, 0.0f); //Rotate the scene
	//Draw a box around the scene
	//glRectf(-14.9f, 9.9f, 14.9f, -9.9f); This is filled!!
	glBegin(GL_LINE_STRIP);
	glVertex2f(-14.9f, 9.9f);
	glVertex2f(14.9f, 9.9f);
	
	//glVertex2f(14.9f, 9.9f);
	glVertex2f(14.9f, -9.9f);
	
	//glVertex2f(14.9f, -9.9f);
	glVertex2f(-14.9f, -9.9f);
	
	//glVertex2f(-14.9f, -9.9f);
	glVertex2f(-14.9f, 9.9f);
	glEnd();
	
	glBegin(GL_LINES);// Start drawing the lines
	for ( uint i = 0; i < m_bands.size(); i++ )
	{	
		// Calculate new horizontal position (x) depending on number of samples
		x = -15.0f + ((30.0f) / float(m_bands.size()) * i);	

		// Calculating new vertical position (y) depending on the data passed by amarok
		y = -10.0f + float(m_bands[i] * 30.0f); //Should multiply by 20 but it looks crappy
		
		if ((y - m_oldy[i]) > 0.5) // Going Up Too Much
		{
			//y = m_oldy[i] + 0.5f;
		}
		else if((y - m_oldy[i]) < -1.1f) // Going Down Too Much
		{
			y = m_oldy[i] - 1.1f;
		}
		
		m_oldy[i] = y; //Save value as last value
			
		//kdDebug() << "Band ["<< i << "] Data: " << m_bands[i] << " (" << x1 << "," << y1 << ") -> (" << x2 << "," << y2 << ")" << endl;
		// Draw Line from new position to old position
		glVertex2f(x, -10.0f);
		glVertex2f(x, y);
  	}
	glEnd();
}

void GLAnalyzer::paintGL()
{	
	glMatrixMode( GL_MODELVIEW );
	glClear( GL_COLOR_BUFFER_BIT |  GL_DEPTH_BUFFER_BIT );
	drawScope();
	glFlush();

}

void GLAnalyzer::interpolate(std::vector<float> *oldVec)
{
    if ( oldVec->size() )
    {    
        uint newSize = m_bands.size(); //vector::size() is O(1)
    
        //necessary? code bloat if not
        if( newSize == oldVec->size() ) { m_bands = *oldVec; return; }
    
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
    
            m_bands[i] = (*oldVec)[indexLeft] * ( 1.0 - error ) + (*oldVec)[indexRight] * error;
        }
    }
}
#include "glanalyzer.moc"
