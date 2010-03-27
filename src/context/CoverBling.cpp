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

#include "core/support/Debug.h"
#include "collection/CollectionManager.h"
#include "core/meta/Meta.h"

#include <math.h>
#ifdef Q_WS_MAC
#include <OpenGL/glext.h>
#else
#include <GL/glext.h>
#endif
#include <QGLFormat>
#include <QGLWidget>
#include <QTimer>

#include <climits>

#define TEXTURE_SIZE QSize( 256, 256 )


CoverBling::CoverBling( QWidget* parent )
        : QGLWidget( QGLFormat(QGL::DepthBuffer|QGL::SampleBuffers|QGL::AlphaChannel|QGL::DoubleBuffer), parent )
        , m_xOffset( 0.0 )
        , m_zOffset( M_PI / 2 )
{
    DEBUG_BLOCK

    setFixedHeight( 200 );

    Amarok::Collection *coll = CollectionManager::instance()->primaryCollection();
    QueryMaker *qm = coll->queryMaker();
    qm->setQueryType( QueryMaker::Album );
    qm->limitMaxResultSize( 10 );

    connect( qm, SIGNAL( newResultReady( QString, Meta::AlbumList ) ), this, SLOT( queryResult( QString, Meta::AlbumList ) ) );

    qm->run();
}

void
CoverBling::queryResult( QString collectionId, Meta::AlbumList albums )
{
    foreach( Meta::AlbumPtr album, albums )
        m_covers << album->image();

    QTimer* timer = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( updateGL() ) );
    timer->start( 20 ); //50fps
}

void
CoverBling::initializeGL() //reimplemented
{
    DEBUG_BLOCK

    //generate all textures
    foreach( const QPixmap &p, m_covers ) {
        QImage image = p.toImage();
        image = image.scaled( TEXTURE_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation );
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
    draw( objectAtPosition( mousePos ) );
}

void
CoverBling::draw( GLuint selected )
{
    GLuint objectName = 1;

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glRotatef( 10, 1.0, 0.0, 0.0 ); //Rotate whole scene around X axis; simulates camera tilt
    glScalef( 1.0, 1.0, 6.0 );

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

    float xoffset = -5.5;
    float yoffset = -0.6;
    float zoffset = -1.1;

    foreach( GLuint id, m_textureIds ) { // krazy:exclude=foreach
        glBindTexture( GL_TEXTURE_2D, id );
        glPushMatrix();
            //const float xsin = sin( xoffset );
            //const float zsin = sin( zoffset );
            xoffset += 1.0;
            zoffset += 0.1;
            glTranslatef( xoffset, yoffset, zoffset );
            glRotatef( 8, 0.0, 1.0, 0.0 );

            //draw the cover
            if( objectName == selected )
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


#include "CoverBling.moc"

