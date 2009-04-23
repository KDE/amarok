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

#ifndef AMAROK_APPLET_H
#define AMAROK_APPLET_H

#include "amarok_export.h"

#include <plasma/applet.h>

#include <QFont>
#include <QRectF>
#include <QString>

class QPainter;

namespace Context
{

class AMAROK_EXPORT Applet : public Plasma::Applet
{

    public:
        explicit Applet( QObject* parent, const QVariantList& args = QVariantList() );

        //helper functions
        QFont shrinkTextSizeToFit( const QString& text, const QRectF& bounds );
        QString truncateTextToFit( QString text, const QFont& font, const QRectF& bounds );

        void drawRoundedRectAroundText( QPainter* p, QGraphicsSimpleTextItem* t );
        void addGradientToAppletBackground( QPainter* p );

        /**
          * Returns a standard CV-wide padding that applets can use for consistency.
          */
        qreal standardPadding();
        
    public Q_SLOTS:
        virtual void destroy();

    private:
        void cleanUpAndDelete();

        bool m_transient;
        qreal m_standardPadding;
        
};

} // Context namespace

/**
 * Register an applet when it is contained in a loadable module
 */
#define K_EXPORT_AMAROK_APPLET(libname, classname) \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(factory("amarok_context_applet_" #libname))\
K_EXPORT_PLUGIN_VERSION(PLASMA_VERSION)

#endif // multiple inclusion guard
