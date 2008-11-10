/*
 * Copyright 2008 by Rob Scheepmaker <r.scheepmaker@student.utwente.nl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef PLASMA_EXTENDER_H
#define PLASMA_EXTENDER_H

#include <QtGui/QGraphicsWidget>

#include "plasma/framesvg.h"
#include "plasma/plasma_export.h"

namespace Plasma
{
class ExtenderPrivate;
class ExtenderItem;
class Applet;

/**
 * @class Extender plasma/extender.h <Plasma/Extender>
 *
 * @short Extends applets to allow detachable parts
 *
 * An Extender is a widget that visually extends the normal contents of an applet with
 * additional dynamic widgets called ExtenderItems. These ExtenderItems can be
 * detached by the user and dropped either on another Extender or on the canvas directly.
 *
 * This widget allows using ExtenderItems in your applet. Extender takes care of the presentation
 * of a collection of ExtenderItems and keeps track of ExtenderItems that originate in it.
 *
 * The default Extender implementation displays extender items in a vertical layout with
 * spacers that appear when dropping an ExtenderItem over it.
 *
 * If you wish to have a different presentation of extender items, you can choose to subclass
 * Extender and reimplement the extenderItem* events and, optionally, the saveState function.
 *
 * To use an Extender in you applet, you'll have to instantiate one. A call to extender() in your
 * applet will create an extender on your applet if you haven't got one already. Every applet can
 * contain only one extender. Think of it as a decorator that adds some functionality to applets
 * that require it. Never instantiate an Extender before init() in your applet. This won't work
 * correctly since a scene is required when an Extender is instantiated.
 *
 * As soon as an Extender is instantiated, ExtenderItems contained previously in this Extender are
 * restored using the initExtenderItem function from the applet the items originally came from. For
 * more information on how this works and how to use ExtenderItems in general, see the ExtenderItem
 * API documentation.
 */
class PLASMA_EXPORT Extender : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY(QString emptyExtenderMessage READ emptyExtenderMessage WRITE setEmptyExtenderMessage)

    public:
        /**
         * Description on how to render the extender's items.
         */
        enum Appearance {
            NoBorders = 0,  /**< Draws no borders on the extender's items. When placed in an applet
                                 on the desktop, use this setting and use the standard margins of
                                 the applet containing this extender. */
            BottomUpStacked = 1, /**< Draws no borders on the topmost extenderitem, but draws the
                                      left, top and right border on subsequent items. When margins
                                      of the containing dialog are set to 0, except for the top
                                      margin, this leads to the 'stacked' look, recommended for
                                      extenders of applet's contained in a panel at the bottom of
                                      the screen. */
            TopDownStacked = 2 /**< Draws no borders on the bottom extenderitem, but draws the
                                    left, bottom and right border on subsequent items. When margins
                                    of the containing dialog are set to 0, except for the bottom
                                    margin, this leads to the 'stacked' look, recommended for
                                    extenders of applet's contained in a panel at the top of
                                    the screen. */
        };

        /**
         * Creates an extender. Note that extender expects applet to have a config(), and needs a
         * scene because of that. So you should only instantiate an extender in init() or later, not
         * in an applet's constructor.
         * The constructor also takes care of restoring ExtenderItems that were contained in this
         * extender before, so ExtenderItems are persistent between sessions.
         * @param applet The applet this extender is part of. Null is not allowed here.
         */
        explicit Extender(Applet *applet);

        ~Extender();

        /**
         * @param message The text to be shown whenever the applet's extender is empty.
         * Defaults to i18n'ed "no items".
         */
        void setEmptyExtenderMessage(const QString &message);

        /**
         * @return The text to be shown whenever the applet's layout is empty.
         */
        QString emptyExtenderMessage() const;

        /**
         * @returns a list of all extender items (attached AND detached) where the source applet is
         * this applet.
         */
        QList<ExtenderItem*> items() const;

        /**
         * @returns a list of all attached extender items.
         */
        QList<ExtenderItem*> attachedItems() const;

        /**
         * @returns a list of all detached extender items.
         */
        QList<ExtenderItem*> detachedItems() const;

        /**
         * This function can be used for easily determining if a certain item is already displayed
         * in a extender item somewhere, so your applet doesn't duplicate this item. Say the applet
         * displays 'jobs', from an engine which add's a source for every job. In sourceAdded you
         * could do something like:
         * if (!item(source)) {
         *     //add an extender item monitoring this source.
         * }
         */
        ExtenderItem *item(const QString &name) const;

        /**
         * Use this function to instruct the extender on how to render it's items. Usually you will
         * want to call this function in your applet's constraintsEvent, allthough this is already
         * done for you when using PopupApplet at base class for your applet. Defaults to NoBorders.
         */
        void setExtenderAppearance(Appearance appearance);

        /**
         * @return the current way of rendering extender items that is used.
         */
        Appearance extenderAppearance() const;

    protected:
        /**
         * Get's called after an item has been added to this extender. The bookkeeping has already
         * been done when this function get's called. The only thing left to do is put it somewhere
         * appropriate. The default implementation adds the extenderItem to the appropriate place in
         * a QGraphicsLinearLayout.
         * @param item The item that has just been added.
         * @param pos The location the item has been dropped in local coordinates.
         */
        virtual void itemAddedEvent(ExtenderItem *item, const QPointF &pos);

        /**
         * Get's called after an item has been removed from this extender. All bookkeeping has
         * already been done when this function get's called.
         * @param item The item that has just been removed.
         */
        virtual void itemRemovedEvent(ExtenderItem *item);

        /**
         * Get's called when an ExtenderItem that get's dragged enters this extender. Default
         * implementation does nothing.
         */
        virtual void itemHoverEnterEvent(ExtenderItem *item);

        /**
         * Gets called when an ExtenderItem is hovering over this extender. Implement this function
         * to give some visual feedback about what will happen when the mouse button is released at
         * that position. The default implementation shows a spacer at the appropriate location in
         * the layout.
         * @param item The item that's hovering over this extender. Most useful for obtaining the
         * size of the spacer.
         * @param pos The location the item is hovering.
         */
        virtual void itemHoverMoveEvent(ExtenderItem *item, const QPointF &pos);

        /**
         * Get's called when an ExtenderItem that was previously hovering over this extender moves
         * away from this extender. The default implementation removes any spacer from the layout.
         */
        virtual void itemHoverLeaveEvent(ExtenderItem *item);

        /**
         * This function get's called for every extender when plasma exits. Implement this function
         * to store the current state of this extender (position in a layout for example), so this
         * can be restored when applet starts again. The default implementation stores the y
         * coordinate of every extender item in the config field extenderItemPos.
         */
        virtual void saveState();

        /**
         * This function get's called on every item to determine which background border's to
         * render.
         * @param item the item for which it's position or extender has changed.
         * @return the borders that have to be enabled on it's background.
         */
        virtual FrameSvg::EnabledBorders enabledBordersForItem(ExtenderItem *item) const;

        /**
         * Reimplemented from QGraphicsWidget
         */
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);

        /**
         * Reimplemented from QGraphicsWidget
         */
        void resizeEvent(QGraphicsSceneResizeEvent *event);

    Q_SIGNALS:
        /**
         * Fires when an extender item is added to this extender.
         */
        void itemAttached(Plasma::ExtenderItem *);

        /**
         * Fires when an extender item is removed from this extender.
         */
        void itemDetached(Plasma::ExtenderItem *);

        /**
         * Fires when an extender's preferred size changes.
         */
        void geometryChanged();

    private:
        ExtenderPrivate *const d;

        friend class ExtenderPrivate;
        friend class ExtenderItem;
        friend class ExtenderItemPrivate;
        //dialog needs access to the extender's applet location.
        friend class DialogPrivate;
        //applet should be able to call saveState();
        friend class Applet;

    };
} // Plasma namespace

#endif //PLASMA_EXTENDER_H

