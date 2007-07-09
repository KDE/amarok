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

#include "debug.h"
#include "CoverBling.h"

#include <math.h>

#include <QtOpenGL>
#include <KStandardDirs>


CoverBling::CoverBling( QWidget* parent )
        : QGLWidget( parent )
        , m_xOffset( 0.0 )
        , m_zOffset( M_PI / 2 )
{
    DEBUG_BLOCK

    setFixedHeight( 200 );

    QTimer* timer = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( updateGL() ) );
    timer->start( 20 ); //50fps
}

void
CoverBling::initializeGL() //reimplemented
{
    DEBUG_BLOCK

    const QImage image( KStandardDirs().findResource("data", "amarok/images/splash_screen.jpg") );
    m_textureWidth = image.width();
    m_textureHeight = image.height();
    m_textureId = bindTexture( image );

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glShadeModel(GL_SMOOTH); 
    glEnable( GL_TEXTURE_2D);
    qglClearColor( Qt::blue );
}

void
CoverBling::paintGL() //reimplemented
{
    //const int mousex = QCursor::pos().x();

    float xoffset = sin( m_xOffset ) / 2;
    float zoffset = sin( m_zOffset ) / 3;
    //debug() << xoffset << endl;
    m_xOffset += 0.03;
    m_zOffset += 0.03;

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glLoadIdentity();

    //glRotatef( xoffset * 100, 0.0, 1.0, 0.0 );
    //glScalef( 1.0, 1.0, zoffset );
    glTranslatef( xoffset, 0.0, zoffset );

    glBegin (GL_QUADS);
    {
        glTexCoord2f (0.0, 0.0);
        glVertex3f (-1.0, -1.0, -1.0);
        glTexCoord2f (1.0, 0.0);
        glVertex3f (1.0, -1.0, -1.0);
        glTexCoord2f (1.0, 1.0);
        glVertex3f (1.0, 1.0, -1.0);
        glTexCoord2f (0.0, 1.0);
        glVertex3f (-1.0, 1.0, -1.0);
    }
    glEnd ();
}

void
CoverBling::resizeGL( int width, int height )
{
    DEBUG_BLOCK

    glViewport( 0, 0, (GLint)width, (GLint)height );
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum( -0.5f, 0.5f, -0.5f, 0.5f, 0.3f, 4.5f );
    glMatrixMode(GL_MODELVIEW);
}


#include "CoverBling.moc"

