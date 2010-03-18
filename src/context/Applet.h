/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2010 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef AMAROK_APPLET_H
#define AMAROK_APPLET_H

#include "amarok_export.h"

#include <plasma/applet.h>

#include <QFont>
#include <QRectF>
#include <QString>

class QPainter;

namespace Plasma
{
    class FrameSvg;
    class IconWidget;
}

namespace Context
{

class AMAROK_EXPORT Applet : public Plasma::Applet
{
    Q_OBJECT
    public:
        explicit Applet( QObject* parent, const QVariantList& args = QVariantList() );
        ~Applet();

        /**
         * Return a QFont that will allow the given text to fit within the rect.
         */
        QFont shrinkTextSizeToFit( const QString& text, const QRectF& bounds );

        /**
         * Truncate the text by adding an ellipsis at the end in order to make the text with the given font
         *  fit in the bounding rect.
         */
        QString truncateTextToFit( QString text, const QFont& font, const QRectF& bounds );

        /**
         * Paint the background for a text label. May or may not actually be a rounded rect, name is obsolete.
         *  Use for the titles of applets, or other heading text.
         */
        void drawRoundedRectAroundText( QPainter* p, QGraphicsTextItem* t );

        /**
         * Paint the background of an applet, so it fits with all the other applets.
         *  Background is *no longer a gradient*. However, please use this to stay consistent with other applets.
         */
        void addGradientToAppletBackground( QPainter* p );

        /**
          * Returns a standard CV-wide padding that applets can use for consistency.
          */
        qreal standardPadding();

        /**
          * Collapse animation
          */
        void setCollapseOn();
        void setCollapseOff();
        void setCollapseHeight( int );

        bool isAppletCollapsed();
        bool isAppletExtended();

        /**
          * Return version of the applet, which is used for sorting out deprecated applets.
            @return version number of the applet
          */
        virtual int appletVersion() const = 0;

        /**
          * sizeHint is reimplemented here only for all the applet.
          */
        virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;

        /**
          * resize is reimplemented here is reimplemented here only for all the applet.
          */
        virtual void   resize( qreal, qreal );

    public Q_SLOTS:
        virtual void destroy();
        void animateOn( qreal );
        void animateOff( qreal );
        void animateEnd( int );

    private slots:
        void paletteChanged( const QPalette & palette );

    protected:
        Plasma::IconWidget* addAction( QAction *action, const int size = 16 );
        bool canAnimate();

        bool m_canAnimate;
        bool m_collapsed;
        int  m_heightCurrent;
        int  m_heightCollapseOn;
        int  m_heightCollapseOff;
        int  m_animationIdOn;
        int  m_animationIdOff;
        int  m_animFromHeight;

    private:
        void cleanUpAndDelete();

        bool m_transient;
        qreal m_standardPadding;
        Plasma::FrameSvg *m_textBackground;
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
