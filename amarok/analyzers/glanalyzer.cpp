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

#include <config.h>

#include "glanalyzer.h"

#include <math.h>
#include <vector>

#include <kdebug.h>
#include <kstandarddirs.h>

GLAnalyzer::GLAnalyzer( QWidget *parent, const char *name ):
AnalyzerBase3d(15, parent, name)
{
}

GLAnalyzer::~GLAnalyzer()
{
}

// METHODS =====================================================

void GLAnalyzer::init()
{
  	x = y = -10.0f;
      	m_bands.resize(20 , 0.0f);
	m_oldy.resize(20, -10.0f);
	m_peaks.resize(20);
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
	GLfloat position0 [] = { 0.0f, 0.0f, -50.0f, 0.0f };
	GLfloat position1 [] = { 0.0f, 0.0f, 50.0f, 0.0f };
	GLfloat position2 [] = { 20.0f, 10.0f, -50.0f, 0.0f };
	GLfloat position3 [] = { 20.0f, 10.0f, 50.0f, 0.0f };
	
	GLfloat colour0 [] = { 1.0, 1.0, 1.0, 0.0 };
	GLfloat colour1 [] = { 1.0, 0.0, 0.0, 0.0 };
	GLfloat colour2 [] = { 0.0, 1.0, 0.0, 0.0 };
	GLfloat colour3 [] = { 0.0, 0.0, 1.0, 0.0 };
	
	GLfloat specular [] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat shininess [] = { 100.0 };
	
	init();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);// Set clear color to black
	// Set the shading model
	glShadeModel(GL_SMOOTH);

	// Set the polygon mode to fill
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Enable depth testing for hidden line removal
	glEnable(GL_DEPTH_TEST);	
	
	/*
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
	
	// Create a Directional Light Source 0
	glLightfv(GL_LIGHT0, GL_POSITION, position0);
	//glLightfv(GL_LIGHT0, GL_AMBIENT, colour0);
	glEnable(GL_LIGHT0);
	
	glLightfv(GL_LIGHT1, GL_POSITION, position1);
	//glLightfv(GL_LIGHT1, GL_AMBIENT, colour1);
	glEnable(GL_LIGHT1);
	
	glLightfv(GL_LIGHT2, GL_POSITION, position2);
	//glLightfv(GL_LIGHT2, GL_DIFFUSE, colour2);
	glEnable(GL_LIGHT2);
	
	glLightfv(GL_LIGHT3, GL_POSITION, position3);
	//glLightfv(GL_LIGHT3, GL_DIFFUSE, colour3);
	glEnable(GL_LIGHT3);
	
	glEnable(GL_LIGHTING);
	//Switch lights on
	//glEnable(GL_LIGHTING);
	//glLightf(GL_LIGHT0, GL_AMBIENT, (1.0f, 1.0f, 1.0f, 1.0f));
	*/
}

void GLAnalyzer::resizeGL( int w, int h )
{
    glViewport( 0, 0, (GLint)w, (GLint)h );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho(-20.0f, 20.0f, -10.0f, 10.0f, -50.0f, 100.0f);
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
}
	
void GLAnalyzer::drawScope()
{
	//kdDebug() << "GLAnalyzer::drawScope()" << endl;
	glRotatef(0.25f, 0.1f, 1.0f, 0.0f); //Rotate the scene
	for ( uint i = 0; i < m_bands.size(); i++ )
	{	
		// Calculate new horizontal position (x) depending on number of samples
		x = -20.0f + ((40.0f) / float(m_bands.size()) * i);	

		// Calculating new vertical position (y) depending on the data passed by amarok
		y = float(m_bands[i] * 30.0f); //Should multiply by 20 but it looks crappy
		
		if((y - m_oldy[i]) < -0.5f) // Going Down Too Much
		{
			y = m_oldy[i] - 0.5f;
		}
		if (y < 0.0f)
		{
			y = 0.0f;
		}
		
		m_oldy[i] = y; //Save value as last value
		
		//Peak Code
		if (m_oldy[i] > m_peaks[i].level)
		{
			m_peaks[i].level = m_oldy[i];
			m_peaks[i].delay = 30;
		}
		
		if (m_peaks[i].delay > 0)
		{
			m_peaks[i].delay--;
		}
		
		if (m_peaks[i].level > 1.0f)
		{
			if (m_peaks[i].delay <= 0)
			{
				m_peaks[i].level-=0.3f;
			}
		}
    	
		// Draw the bar
		drawBar(x,y);
		drawPeak(x, m_peaks[i].level);
  	}
	swapBuffers();
	/*glEnd();*/
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

void GLAnalyzer::drawBar(float xPos, float height)
{
        glPushMatrix();

        /*Sets color to blue*/
        //glColor3f(0.5f, 0.625f, 1.0f);
	glColor3f((height/40) + 0.5f, (height/40) + 0.625f, 1.0f);
        glTranslatef(xPos, -10.0f, 0.0f);
                
        glScalef(1.0f, 0-height, 3.0f);
        drawCube();
	drawFrame();
        glPopMatrix();
}

void GLAnalyzer::drawPeak(float xPos, float ypos)
{
        glPushMatrix();

        /*Sets color to blue*/
        //glColor3f(0.5f, 0.625f, 1.0f);
	glColor3f(1.0f, 0.0f, 0.0f);
        glTranslatef(xPos, ypos - 9.0f, 0.0f);
                
        glScalef(1.0f, 1.0f, 3.0f);
        drawCube();

        glPopMatrix();
}

void GLAnalyzer::drawCube()
{
        glPushMatrix();
        glBegin(GL_POLYGON);

                /*      This is the top face*/
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(0.0f, 0.0f, -1.0f);
                glVertex3f(-1.0f, 0.0f, -1.0f);
                glVertex3f(-1.0f, 0.0f, 0.0f);

                /*      This is the front face*/
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(-1.0f, 0.0f, 0.0f);
                glVertex3f(-1.0f, -1.0f, 0.0f);
                glVertex3f(0.0f, -1.0f, 0.0f);

                /*      This is the right face*/
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(0.0f, -1.0f, 0.0f);
                glVertex3f(0.0f, -1.0f, -1.0f);
                glVertex3f(0.0f, 0.0f, -1.0f);

                /*      This is the left face*/
                glVertex3f(-1.0f, 0.0f, 0.0f);
                glVertex3f(-1.0f, 0.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, 0.0f);

                /*      This is the bottom face*/
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(0.0f, -1.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, 0.0f);

                /*      This is the back face*/
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(-1.0f, 0.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, -1.0f);
                glVertex3f(0.0f, -1.0f, -1.0f);

        glEnd();
        glPopMatrix();
}
void GLAnalyzer::drawFrame()
{
        glPushMatrix();
        glBegin(GL_LINES);
		glColor3f(0.0f, 0.0f, 1.0f);

                /*      This is the top face*/
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(0.0f, 0.0f, -1.0f);
                glVertex3f(-1.0f, 0.0f, -1.0f);
                glVertex3f(-1.0f, 0.0f, 0.0f);

                /*      This is the front face*/
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(-1.0f, 0.0f, 0.0f);
                glVertex3f(-1.0f, -1.0f, 0.0f);
                glVertex3f(0.0f, -1.0f, 0.0f);

                /*      This is the right face*/
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(0.0f, -1.0f, 0.0f);
                glVertex3f(0.0f, -1.0f, -1.0f);
                glVertex3f(0.0f, 0.0f, -1.0f);

                /*      This is the left face*/
                glVertex3f(-1.0f, 0.0f, 0.0f);
                glVertex3f(-1.0f, 0.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, 0.0f);

                /*      This is the bottom face*/
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(0.0f, -1.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, 0.0f);

                /*      This is the back face*/
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(-1.0f, 0.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, -1.0f);
                glVertex3f(0.0f, -1.0f, -1.0f);

        glEnd();
        glPopMatrix();
}
#include "glanalyzer.moc"
