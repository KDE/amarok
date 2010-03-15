/****************************************************************************************
 * Copyright (c) 2007 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "CoverBling"

#include "CoverBling.h"

#include "Debug.h"
#include "collection/CollectionManager.h"
#include "meta/Meta.h"

#include <math.h>
#ifdef Q_WS_MAC
#include <OpenGL/glext.h>
#else
#include <GL/glext.h>
#endif
#include <QGLFormat>
#include <QGLWidget>
#include <QTimer>
#include "QMouseEvent"

#include <climits>
#include "ImageLoader.h"
#include <iostream>

CoverBling::CoverBling( QWidget* parent,Meta::AlbumList albums )
        : QGLWidget( QGLFormat(QGL::DepthBuffer|QGL::SampleBuffers|QGL::AlphaChannel|QGL::DoubleBuffer), parent )
        , m_xOffset( 0.0 )
        , m_zOffset( M_PI / 2 )
{
    DEBUG_BLOCK

	m_currentindex = -1;
	makeCurrent();
	m_animationDuration = 20;
	m_coversize = QSize (150,150);
    //setFixedHeight( 300 );
	queryResult("",albums);
	m_animationStep = 0;
	m_animation_StepMax = 10;
	m_anim_forward = true;
}

void
CoverBling::queryResult( QString collectionId, Meta::AlbumList albums )
{
	m_albums = albums;
	m_timer = new QTimer( this );
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( updateGL() ) );
    m_timer->start( 20 ); //50fps
    
    connect(&animateTimer, SIGNAL( timeout() ), this, SLOT(updateAnimation()));
}
void CoverBling::init(Meta::AlbumList albums,QSize iSize)
{
	queryResult("",albums);
	m_coversize = iSize;
}
void
CoverBling::initializeGL() //reimplemented
{
    DEBUG_BLOCK
    if (m_timer) m_timer->stop();
    //generate all textures
    foreach( Meta::AlbumPtr album, m_albums ) {
		QImage image = PlainImageLoader::loadAndResize( album, m_coversize );
        m_textureIds << bindTexture( image );
    }
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glShadeModel(GL_SMOOTH); 
    qglClearColor( Qt::black );
    glEnable( GL_MULTISAMPLE ); //enable anti aliasing
    glEnable( GL_DEPTH_TEST );
    glDepthMask( true );
 
    //Display list for drawing a textured rectangle
    m_texturedRectList = glGenLists( 1 );
    glNewList( m_texturedRectList, GL_COMPILE );
        glBegin (GL_QUADS);
            glTexCoord2f (0.0, 0.0);
            glColor3f( 1.0, 1.0, 1.0 );
            glVertex3f (-1.0, -1.0, -1.0);
            glTexCoord2f (1.0, 0.0);
            glColor3f( 0.1, 0.1, 0.1 );
            glVertex3f (1.0, -1.0, -1.0);
            glTexCoord2f (1.0, 1.0);
            glColor3f( 0.1, 0.1, 0.1 );
            glVertex3f (1.0, 1.0, -1.0);
            glTexCoord2f (0.0, 1.0);
            glColor3f( 1.0, 1.0, 1.0 );
            glVertex3f (-1.0, 1.0, -1.0);
        glEnd ();
        //glDisable( GL_DEPTH_TEST );
    glEndList();

    //Display list for drawing reflection of the textured rectangle
    m_texturedRectReflectedList = glGenLists( 1 );
    glNewList( m_texturedRectReflectedList, GL_COMPILE );
        glTranslatef( 0.0, -2.0, 0.0 );
        glScalef( 1.0, -1.0, 1.0 );

        glEnable( GL_BLEND );
        //glBlendFunc( GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        glBegin (GL_QUADS);
            glTexCoord2f (0.0, 0.0);
            glColor4f( 1.0, 1.0, 1.0, 0.3 );
            glVertex3f (-1.0, -1.0, -1.0);
            glColor4f( 1.0, 1.0, 1.0, 0.3 - 0.15 );
            glTexCoord2f (1.0, 0.0);
            glVertex3f (1.0, -1.0, -1.0);
            glColor4f( 1.0, 1.0, 1.0, 0.02 - 0.15 );
            glTexCoord2f (1.0, 1.0);
            glVertex3f (1.0, 1.0, -1.0);
            glColor4f( 1.0, 1.0, 1.0, 0.02 );
            glTexCoord2f (0.0, 1.0);
            glVertex3f (-1.0, 1.0, -1.0);
        glEnd ();

        glDisable( GL_BLEND );
    glEndList();
    
}

void
CoverBling::resizeGL( int width, int height ) //reimplemented
{
    DEBUG_BLOCK

    glViewport( 0, 0, (GLint)width, (GLint)height );
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //glFrustum( -0.5f, 0.5f, -0.5f, 0.5f, 0.3f, 4.5f );
    setPerspective();
    glMatrixMode(GL_MODELVIEW);
}

void
CoverBling::setPerspective()
{
    gluPerspective( 30, (double)width() / height(), 1.0, 20.0 );
}

void
CoverBling::paintGL() //reimplemented
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    const QPoint mousePos = mapFromGlobal( QCursor::pos() );
    draw(0);
    //draw( objectAtPosition( mousePos ) );
}
void
CoverBling::draw( GLuint selected )
{
	//DEBUG_BLOCK
	
    GLuint objectName = 1;

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glRotatef( 4, 1.0, 0.0, 0.0 ); //Rotate whole scene around X axis; simulates camera tilt
    glScalef( 1.0, 1.0, 6.0 );
    float step = ((float)m_animationStep)/((float)m_animation_StepMax);    	
    if (!animateTimer.isActive()) step=1; 
    
	//glTranslatef( -2*m_currentindex*step, 0.0, 0.0 );			
   
    glColor3f( 1.0, 1.0, 1.0 ); //reset color
    glEnable( GL_TEXTURE_2D);
    float xoffset = 0.5;
    float yoffset = -0.6;
    float zoffset = -1.1;
	int nbtextures = m_textureIds.size();
	for (int i=0;i<nbtextures;i++)
	{
		GLuint id = m_textureIds[i];
        glBindTexture( GL_TEXTURE_2D, id );
        glPushMatrix();
            
            xoffset += 1.0;
            zoffset += 0.1;
            int idx_diff = m_currentindex-i;
            GLdouble step = ((GLdouble)m_animationStep)/((GLdouble)m_animation_StepMax);
            double angle_rot = 10;    	
			if (!animateTimer.isActive()) step=1;			 
			//if (!m_anim_forward) idx_diff = -idx_diff;
			int forward_fac = 1;
			if (!m_anim_forward) forward_fac = -1;
			if (m_anim_forward)
			{
				if (idx_diff==0 || idx_diff==-1)
				{
					if (i==m_currentindex+1)
					{
						glTranslatef( -2*idx_diff*(1-step), 0.0, -1*(1-step) );
						glRotatef( -angle_rot*(1-step), 0.0, 1.0, 0.0 );
					}
					if (i==m_currentindex)
					{
						glTranslatef( -2*(step), 0.0, -1*step );
						glRotatef( angle_rot*step, 0.0, 1.0, 0.0 );
					}
				}
				else
				{
					double fac = idx_diff/(idx_diff*idx_diff+1);
				if (idx_diff >0)
				{
					glTranslatef( -2*(idx_diff+1), 0.0, -1 );
					glRotatef( angle_rot, 0.0, 1.0, 0.0 );
				}
				else if (idx_diff<0)
				{
					glTranslatef( -2*idx_diff, 0.0, -1 );
					glRotatef( -angle_rot, 0.0, 1.0, 0.0 );
				}
				}
			}
			else
			{
				if (idx_diff==-2 || idx_diff==-1)
				{
					if (i==m_currentindex+1)
					{
						glTranslatef( 2*idx_diff*(1-step), 0.0, -1*(1-step) );
						glRotatef( angle_rot*(1-step), 0.0, 1.0, 0.0 );
					}
					if (i==m_currentindex+2)
					{
						glTranslatef( 2*(step+1), 0.0, -1*step );
						glRotatef( -angle_rot*step, 0.0, 1.0, 0.0 );
					}
				}
				else
				{
				if (idx_diff >1)
				{
					glTranslatef( -2*(idx_diff), 0.0, -1 );
					glRotatef( -angle_rot, 0.0, 1.0, 0.0 );
				}
				else if (idx_diff<0)
				{
					glTranslatef( -2*(idx_diff), 0.0, -1 );
					glRotatef( -angle_rot, 0.0, 1.0, 0.0 );
				}
				}
			}	
            //draw the cover
			// celle là il faut la mettre à plat au milieu !!!
			//else
                glColor3f( 1.0, 0.0, 0.0 );
            glLoadName( objectName++ );
            glCallList( m_texturedRectList );
            glColor4f( 1.0, 1.0, 1.0, 1.0 );

            //draw reflection on the ground
            glLoadName( 0 );
            glPushMatrix();
                glCallList( m_texturedRectReflectedList );
            glPopMatrix();
        glPopMatrix();
        glColor4f( 1.0, 1.0, 1.0, 1.0 );
	}
    glDisable( GL_TEXTURE_2D);
}

GLuint
CoverBling::objectAtPosition( const QPoint& pos )
{
    // this is the same as in every OpenGL picking example
    const int MaxSize = 512; // see below for an explanation on the buffer content
    GLuint buffer[MaxSize];
    GLint viewport[4];

    glGetIntegerv(GL_VIEWPORT, viewport);
    glSelectBuffer(MaxSize, buffer);
    // enter select mode
    glRenderMode(GL_SELECT);

    glInitNames();
    glPushName(0);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
        glLoadIdentity();
        gluPickMatrix((GLdouble)pos.x(), (GLdouble)(viewport[3] - pos.y()), 5.0, 5.0, viewport);
        setPerspective();
        draw();
        glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    const int hits = glRenderMode( GL_RENDER );
    if ( !hits )
        return 0;

    //determine object with the lowest Z value
    uint hitZValue = UINT_MAX;
    uint hit       = UINT_MAX;
    for( int i = 0; i < hits; i++ ) { 
        if( buffer[(i*4)+1] < hitZValue ) { 
            hit       = buffer[(i*4)+3];
            hitZValue = buffer[(i*4)+1];
        }
    }
    
    // return the name of the clicked surface
    return hit;
}
void CoverBling::mousePressEvent(QMouseEvent *event)
{
	DEBUG_BLOCK
	m_animationStep=0;
	if ( event->x() > ( width() / 2))
	{
        m_currentindex++;
        m_anim_forward = true;
	}
	else
	{
        m_currentindex--;
        m_anim_forward = false;
	}
	if (m_currentindex<0) m_currentindex = 0;
	if (m_currentindex> m_albums.size()) m_currentindex = m_albums.size();
	animateTimer.start(m_animationDuration);
	//updateGL();
}
void CoverBling::setCurrentIndex(int idx)
{
	DEBUG_BLOCK
	m_currentindex = idx;
}
void CoverBling::updateAnimation()
{
	m_animationStep++;
	if (m_animationStep==m_animation_StepMax) {animateTimer.stop();m_animationStep=0;}
	updateGL();
}
#include "CoverBling.moc"

