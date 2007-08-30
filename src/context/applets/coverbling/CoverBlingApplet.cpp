/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "CoverBlingApplet.h"

#include "debug.h"
#include <math.h>

#include <KStandardDirs>

#include <QTimer>

#define TEXTURE_SIZE QSize( 256, 256 )

CoverBlingApplet::CoverBlingApplet( QObject* parent, const QVariantList& args )
    : Plasma::GLApplet( parent, args )
    , m_xOffset( 0.0 )
    , m_zOffset( M_PI / 2 )
{
    DEBUG_BLOCK

    setHasConfigurationInterface( false );
    setDrawStandardBackground( true );

    m_coverPaths << "amarok/images/album_cover_1.jpg";
    m_coverPaths << "amarok/images/album_cover_2.jpg";
    m_coverPaths << "amarok/images/album_cover_3.jpg";
    m_coverPaths << "amarok/images/album_cover_4.jpg";
    m_coverPaths << "amarok/images/album_cover_5.jpg";
    m_coverPaths << "amarok/images/album_cover_6.jpg";
    m_coverPaths << "amarok/images/album_cover_7.jpg";
    m_coverPaths << "amarok/images/album_cover_8.jpg";
    m_coverPaths << "amarok/images/album_cover_9.jpg";

//     QTimer* timer = new QTimer( this );
//     connect( timer, SIGNAL( timeout() ), this, SLOT( update( const QRectF) ) );
//     timer->start( 20 ); //50fps

    //FIXME: this probably shouldn't be here but not sure where else to put it..
       //generate all textures
    foreach( QString path, m_coverPaths ) {
        QImage image( KStandardDirs().findResource( "data", path ) );
        image = image.scaled( TEXTURE_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        m_textureIds << bindTexture( image );
    }

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glShadeModel(GL_SMOOTH);
    //TODO:PORT
//     qglClearColor( Qt::black );
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

CoverBlingApplet::~CoverBlingApplet()
{
    DEBUG_BLOCK
}

void
CoverBlingApplet::resize( qreal width, qreal height )
{
    DEBUG_BLOCK

    glViewport( 0, 0, (GLint)width, (GLint)height );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    //glFrustum( -0.5f, 0.5f, -0.5f, 0.5f, 0.3f, 4.5f );
    setPerspective();
    glMatrixMode( GL_MODELVIEW );
}

void
CoverBlingApplet::setPerspective()
{
    gluPerspective( 30, (double)boundingRect().width() / boundingRect().height(), 1.0, 20.0 );
}

void
CoverBlingApplet::paintGLInterface( QPainter *, const QStyleOptionGraphicsItem * )
{
    DEBUG_BLOCK

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    //TODO:no mapFromGlobal()
//     const QPoint mousePos = mapFromGlobal( QCursor::pos() );
    const QPoint mousePos = QPoint( 1, 1 );
    draw( objectAtPosition(mousePos) );
}

void
CoverBlingApplet::draw( GLuint selected )
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

    foreach( GLuint id, m_textureIds ) {
        glBindTexture( GL_TEXTURE_2D, id );
        glPushMatrix();
        const float xsin = sin( xoffset );
        const float zsin = sin( zoffset );
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
CoverBlingApplet::objectAtPosition( const QPoint& pos )
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
#include "CoverBlingApplet.moc"
