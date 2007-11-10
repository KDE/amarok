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

/****************************************************************************
    NOTE: We extend plasma::applet in order to provide some functionality that
          should be shared between amarok applets. this is to reduce the chances
          of the delicate resize/layouting code not being correctly implemented
          (which are high).

          if you are writing your own plasma applet, please make sure you only override these functions
          if you know what you are doing.
****************************************************************************/

#ifndef AMAROK_APPLET_H
#define AMAROK_APPLET_H

#include "amarok_export.h"
#include <plasma/applet.h>

namespace Context
{

class AMAROK_EXPORT Applet : public Plasma::Applet
{
    Q_OBJECT
 public:
    Applet( QGraphicsItem* parent = 0, const QString& serviceId = QString(), uint appletId = 0);
    Applet( QObject* parent, const QVariantList& args) ;

    void setGeometry( const QRectF& rect );
    QSizeF contentSizeHint() const;

    // NOTE do not override this-- use resizeApplet instead.
    //      this just updates the size with the new aspect-ratio
    //      aware size.
    void resize( qreal oldWidth, qreal aspectRatio );

protected:

    // implement applet-specific resizing stuff here. this means resizing the
    // Plasma::Svg objects that are stored, and anything else that needs doing
    virtual void resizeApplet( qreal newWidth, qreal aspectRatio = 1.0 ) = 0;
    
    QSizeF m_size;
    qreal m_aspectRatio;
};

} // Context namespace

/**
 * Register an applet when it is contained in a loadable module
 */
#define K_EXPORT_AMAROK_APPLET(libname, classname) \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(factory("amarok_context_applet_" #libname))

#endif // multiple inclusion guard
