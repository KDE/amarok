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

#ifndef CURRENT_TRACK_APPLET_H
#define CURRENT_TRACK_APPLET_H

#include "Applet.h"
#include "plasma/glapplet.h"

#include <context/DataEngine.h>

class CoverBlingApplet : public Plasma::GLApplet
{
    Q_OBJECT
public:
    CoverBlingApplet( QObject* parent, const QVariantList& args );
    ~CoverBlingApplet();

    virtual void paintGLInterface( QPainter *painter, const QStyleOptionGraphicsItem *option );

protected:
    void draw( GLuint selected = 0 );
    void resize( qreal width, qreal height );
    void setPerspective();
    GLuint objectAtPosition( const QPoint& pos );
    QSizeF contentSizeHint() const { return QSizeF(400.0, 200.0 ); }

private:
    QStringList m_coverPaths;
    QList<GLuint> m_textureIds;
    GLuint m_texturedRectList;
    GLuint m_texturedRectReflectedList;
    float m_xOffset;
    float m_zOffset;

};

K_EXPORT_AMAROK_APPLET( coverbling, CoverBlingApplet )

#endif
