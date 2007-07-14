/***************************************************************************
 *   Copyright (C) 2007 by Mark Kretschmann <kretschmann@kde.org>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#define DEBUG_PREFIX "CoverBling"

#include "debug.h"
#include "CoverBling.h"

#include <math.h>

#include <QtOpenGL>
#include <KStandardDirs>

#define TEXTURE_SIZE QSize( 256, 256 )


CoverBling::CoverBling( QWidget* parent )
        : QGLWidget( QGLFormat(QGL::DepthBuffer|QGL::SampleBuffers|QGL::AlphaChannel|QGL::DoubleBuffer), parent )
        , m_xOffset( 0.0 )
        , m_zOffset( M_PI / 2 )
{
    DEBUG_BLOCK

    setFixedHeight( 200 );

    m_coverPaths << "amarok/images/album_cover_1.jpg";
    m_coverPaths << "amarok/images/album_cover_2.jpg";
    m_coverPaths << "amarok/images/album_cover_3.jpg";
    m_coverPaths << "amarok/images/album_cover_4.jpg";
    m_coverPaths << "amarok/images/album_cover_5.jpg";
   
    QTimer* timer = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( updateGL() ) );
    timer->start( 20 ); //50fps
}

void
CoverBling::initializeGL() //reimplemented
{
    DEBUG_BLOCK

    //generate all textures
    foreach( QString path, m_coverPaths ) {
        QImage image( KStandardDirs().findResource( "data", path ) );
        image = image.scaled( TEXTURE_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        m_textureIds << bindTexture( image );
    }

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glShadeModel(GL_SMOOTH); 
    qglClearColor( Qt::black );
    glEnable( GL_MULTISAMPLE ); //enable anti aliasing
    glEnable( GL_DEPTH_TEST );
    glDepthMask( TRUE );
 
    //Display list for drawing a textured rectangle
    m_texturedRectList = glGenLists( 1 );
    glNewList( m_texturedRectList, GL_COMPILE );
        glBegin (GL_QUADS);
            glTexCoord2f (0.0, 0.0);
            glVertex3f (-1.0, -1.0, -1.0);
            glTexCoord2f (1.0, 0.0);
            glVertex3f (1.0, -1.0, -1.0);
            glTexCoord2f (1.0, 1.0);
            glVertex3f (1.0, 1.0, -1.0);
            glTexCoord2f (0.0, 1.0);
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
            glColor4f( 1.0, 1.0, 1.0, 0.3 );
            glTexCoord2f (1.0, 0.0);
            glVertex3f (1.0, -1.0, -1.0);
            glColor4f( 1.0, 1.0, 1.0, 0.02 );
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
    glFrustum( -0.5f, 0.5f, -0.5f, 0.5f, 0.3f, 4.5f );
    glMatrixMode(GL_MODELVIEW);
        
    // Get the aspect ratio of the screen
    const float ratio = (float)width / (float)height;
    if ( ratio >= 1.0 ) {
        m_aspectX = 1.0 / ratio;
        m_aspectY = 1.0;
    } else {
        m_aspectX = 1.0;
        m_aspectY = 1.0 * ratio;
    }
}

void
CoverBling::paintGL() //reimplemented
{
    //const int mousex = QCursor::pos().x();

    m_xOffset += 0.02;
    m_zOffset += 0.01;

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glLoadIdentity();
    glRotatef( 5, 1.0, 0.0, 0.0 ); //Rotate whole scene around X axis; simulates camera tilt
    glScalef( m_aspectX, m_aspectY, 1.0 ); //aspect correction

    //draw the ground
    //glBegin( GL_POLYGON );
    //    glColor3f( 0.0, 0.0, 0.5 );
    //    glVertex3f (-3.0, -1.0, -2.0);
    //    glColor3f( 1.0, 0.0, 0.5 );
    //    glVertex3f (3.0, -1.0, -2.0);
    //    glColor3f( 0.0, 1.0, 0.5 );
    //    glVertex3f (3.0, -1.0, 2.0);
    //    glColor3f( 0.0, 0.0, 1.5 );
    //    glVertex3f (-3.0, -1.0, 2.0);
    //glEnd();

    glColor3f( 1.0, 1.0, 1.0 ); //reset color
    glEnable( GL_TEXTURE_2D);

    float xoffset = m_xOffset;
    float zoffset = m_zOffset;

    foreach( GLuint id, m_textureIds ) {
        glBindTexture( GL_TEXTURE_2D, id );
        glPushMatrix();
            const float xsin = sin( xoffset );
            const float zsin = sin( zoffset );
            xoffset += 5.5;
            zoffset += 8.0;
            glRotatef( xsin * 5, 0.0, 1.0, 0.0 );
            glTranslatef( xsin * 2.4, 0.0, zsin / 3 );
            
            //draw the cover
            glCallList( m_texturedRectList );

            //draw reflection on the ground
            glPushMatrix();
                glCallList( m_texturedRectReflectedList );
            glPopMatrix();
        glPopMatrix();
        glColor4f( 1.0, 1.0, 1.0, 1.0 );
    }

    glDisable( GL_TEXTURE_2D);
}


#include "CoverBling.moc"

