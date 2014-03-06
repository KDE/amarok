/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
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

#include <KIcon>
#include <Plasma/Applet>

#include <QFont>
#include <QRectF>
#include <QString>
#include <QWeakPointer>

class QPainter;
class QPropertyAnimation;

namespace Plasma
{
    class IconWidget;
}

namespace Context
{

class AppletHeader;

class AMAROK_EXPORT Applet : public Plasma::Applet
{
    Q_OBJECT
    Q_PROPERTY( int collapseHeight READ collapseHeight WRITE setCollapseHeight )
    Q_PROPERTY( int collapseOffHeight READ collapseOffHeight WRITE setCollapseOffHeight )
    Q_PROPERTY( QString headerText READ headerText WRITE setHeaderText )

    public:
        explicit Applet( QObject* parent, const QVariantList& args = QVariantList() );
        ~Applet();

        /**
         * Return a QFont that will allow the given text to fit within the rect.
         */
        QFont shrinkTextSizeToFit( const QString& text, const QRectF& bounds );

        /**
         * Truncate the text by adding an ellipsis at the end in order to make the text with the given font
         * rit in the bounding rect.
         */
        QString truncateTextToFit( const QString &text, const QFont& font, const QRectF& bounds );

        void paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

        /**
          * Returns a standard CV-wide padding that applets can use for consistency.
          */
        qreal standardPadding();

        /**
         * Creates a header for showing title text and icon widgets, if enabled.
         */
        void enableHeader( bool enable = true );

        /**
         * Adds an action on the left of the header text. A header needs to be
         * created by enableHeader() first.
         * @param action Action to add
         * @return An icon widget created for the action
         */
        Plasma::IconWidget *addLeftHeaderAction( QAction *action );

        /**
         * Adds an action on the right of the header text. A header needs to be
         * created by enableHeader() first.
         * @param action Action to add
         * @return An icon widget created for the action
         */
        Plasma::IconWidget *addRightHeaderAction( QAction *action );

        /**
         * Returns the current header text. If no header exists this will return
         * an empty string.
         */
        QString headerText() const;

        /**
         * Sets the text shown in the header. If no header exists this method
         * does nothing.
         */
        void setHeaderText( const QString &text );

        /**
          * Set the preferred applet height when collapsed.
          * The actual height when collapsed will be constrained by the applet's
          * minimum and maximum heights, as well as the current available height
          * from the containment.
          */
        void setCollapseHeight( int );

        /**
          * Set the preferred applet height when uncollapsed.
          * The actual height when collapsed will be constrained by the applet's
          * minimum and maximum heights, as well as the current available height
          * from the containment. Depending on the vertical size policy, the
          * other applets currently showing, and the aforementioned constraints,
          * the actual height when collapse is off may be different. This is so
          * that, for example, the applet may take up the rest of the space when
          * the size policy is set to Expanding. Setting this height to -1 will
          * tell the layout to give the applet the rest of the available space.
          */
        void setCollapseOffHeight( int );

        /**
         * Preferred applet height when collapsed.
         */
        int collapseHeight() const;

        /**
         * Preferred applet height when uncollapsed.
         */
        int collapseOffHeight() const;

        /**
         * Whether a collapse animation is currently running.
         */
        bool isAnimating() const;

        /**
         * Whether the applet is currently collapsed to collapseHeight().
         */
        bool isCollapsed() const;

        /**
         * Shows a warning dialog which blocks access to the applet.
         * This gives the user the message and a "Yes" and a "No" button.
         * NOTE: Only one message/warning can be shown at a time.
         *
         * @param message The warning message.
         * @param slot The slot which is called after either "Yes" or "No" has been clicked.
         */
        void showWarning( const QString &message, const char *slot );

    public Q_SLOTS:
        virtual void destroy();

        /**
         * Collapse to collapseHeight().
         */
        void setCollapseOn();

        /**
         * Collapse to collapseOffHeight().
         */
        void setCollapseOff();

    protected:
        /**
         * Paint the background of an applet, so it fits with all the other applets.
         *  Background is *no longer a gradient*. However, please use this to
         *  stay consistent with other applets.
         */
        void addGradientToAppletBackground( QPainter* p ); // TODO check the applets for this

        Plasma::IconWidget* addAction( QGraphicsItem *parent, QAction *action, const int size = 16 );
        bool canAnimate();

        bool m_canAnimate;
        int  m_heightCurrent;
        int  m_heightCollapseOn;
        int  m_heightCollapseOff;

        AppletHeader *m_header;


    private slots:
        void collapseAnimationFinished();
        void collapse( bool on );

    private:
        void cleanUpAndDelete();

        bool m_transient;
        qreal m_standardPadding;
        QWeakPointer<QPropertyAnimation> m_animation;
};

} // Context namespace

/**
 * Register an applet when it is contained in a loadable module
 */
#define AMAROK_EXPORT_APPLET(libname, classname) \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(factory("amarok_context_applet_" #libname))\
K_EXPORT_PLUGIN_VERSION(PLASMA_VERSION)

#endif // multiple inclusion guard
