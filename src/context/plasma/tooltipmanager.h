/*
 * Copyright 2007 by Dan Meltzer <hydrogen@notyetimplemented.com>
 * Copyright 2008 by Aaron Seigo <aseigo@kde.org>
 * Copyright 2008 by Alexis Ménard <darktears31@gmail.com>
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

#ifndef PLASMA_TOOLTIP_MANAGER_H
#define PLASMA_TOOLTIP_MANAGER_H

//plasma
#include <plasma/plasma.h>
#include <plasma/plasma_export.h>

namespace Plasma
{

class ToolTipManagerPrivate;
class Applet;
class Corona;

/**
 * @class ToolTipManager plasma/tooltipmanager.h <Plasma/ToolTipManager>
 *
 * @short Manages tooltips for QGraphicsWidgets in Plasma
 *
 * If you want a widget to have a tooltip displayed when the mouse is hovered over
 * it, you should do something like:
 *
 * @code
 * // widget is a QGraphicsWidget*
 * Plasma::ToolTipManager::Content data;
 * data.mainText = i18n("My Title");
 * data.subText = i18n("This is a little tooltip");
 * data.image = KIcon("some-icon").pixmap(IconSize(KIconLoader::Desktop));
 * Plasma::ToolTipManager::self()->setContent(widget, data);
 * @endcode
 *
 * Note that, since a Plasma::Applet is a QGraphicsWidget, you can use
 * Plasma::ToolTipManager::self()->setContent(this, data); in the
 * applet's init() method to set a tooltip for the whole applet.
 *
 * The tooltip will be registered automatically by setContent().  It will be
 * automatically unregistered when the associated widget is deleted, freeing the
 * memory used by the tooltip, but you can manually unregister it at any time by
 * calling unregisterWidget().
 *
 * When a tooltip for a widget is about to be shown, the widget's toolTipAboutToShow slot will be
 * invoked if it exists. Similarly, when a tooltip is hidden, the widget's toolTipHidden() slot
 * will be invoked if it exists. This allows widgets to provide on-demand tooltip data.
 */
class PLASMA_EXPORT ToolTipManager  : public QObject
{
    Q_OBJECT
public:

    enum State {
        Activated = 0 /**<< Will accept tooltip data and show tooltips */,
        Inhibited /**<< Will accept tooltip data, but not show tooltips */,
        Deactivated /**<< Will discard tooltip data, and not attempt to show them */
    };

    /**
     * @struct Content plasma/tooltipmanager.h <Plasma/ToolTipManager>
     *
     * This provides the content for a tooltip.
     *
     * Normally you will want to set at least the @p mainText and
     * @p subText.
     */
    struct PLASMA_EXPORT Content
    {
        /** Creates an empty Content */
        Content();

        /** @return true if all the fields are empty */
        bool isEmpty() const;

        /** Important information, e.g. the title */
        QString mainText;
        /** Elaborates on the @p mainText */
        QString subText;
        /** An icon to display */
        QPixmap image;
        /** Id of a window if you want to show a preview */
        WId windowToPreview;
        /** Whether or not to autohide the tooltip, defaults to true */
        bool autohide;
    };

    /**
     * @return The singleton instance of the manager.
     */
    static ToolTipManager *self();

    /**
     * Show the tooltip for a widget registered in the tooltip manager
     *
     * @param widget the widget for which the tooltip will be displayed
     */
    void show(QGraphicsWidget *widget);

    /**
     * Find out whether the tooltip for a given widget is currently being displayed.
     *
     * @param widget the widget to check the tooltip for
     * @return true if the tooltip of the widget is currently displayed,
     *         false if not
     */
    bool isVisible(QGraphicsWidget *widget) const;

    /**
     * Hides the tooltip for a widget immediately.
     *
     * @param widget the widget to hide the tooltip for
     */
    void hide(QGraphicsWidget *widget);

    /**
     * Registers a widget with the tooltip manager.
     *
     * Note that setContent() will register the widget if it
     * has not already been registered, and so you do not normally
     * need to use the method.
     *
     * This is useful for creating tooltip content on demand.  You can
     * register your widget with registerWidget(), then implement
     * a slot named toolTipAboutToShow for the widget.  This will be
     * called before the tooltip is shown, allowing you to set the
     * data with setContent().
     *
     * If the widget also has a toolTipHidden slot, this will be called
     * after the tooltip is hidden.
     *
     * @param widget the desired widget
     */
    void registerWidget(QGraphicsWidget *widget);

    /**
     * Unregisters a widget from the tooltip manager.
     *
     * This will free the memory used by the tooltip associated with the widget.
     *
     * @param widget the desired widget to delete
     */
    void unregisterWidget(QGraphicsWidget *widget);

    /**
     * Sets the content for the tooltip associated with a widget.
     *
     * Note that this will register the widget with the ToolTipManager if
     * necessary, so there is usually no need to call registerWidget().
     *
     * @param widget the widget the tooltip should be associated with
     * @param data   the content of the tooltip. If an empty Content
     *               is passed in, the tooltip content will be reset.
     */
    void setContent(QGraphicsWidget *widget,
                    const ToolTipManager::Content &data);

    /**
     * Clears the tooltip data associated with this widget, but keeps
     * the widget registered.
     */
    void clearContent(QGraphicsWidget *widget);

    /**
     * Sets the current state of the manager.
     * @see State
     * @arg state the state to put the manager in
     */
    void setState(ToolTipManager::State state);

    /**
     * @return the current state of the manager; @see State
     */
    ToolTipManager::State state() const;

private:
    /**
     * Default constructor.
     *
     * You should normall use self() instead.
     */
    explicit ToolTipManager(QObject *parent = 0);

    /**
     * Default destructor.
     */
    ~ToolTipManager();

    friend class ToolTipManagerSingleton;
    friend class Corona; // The corona needs to register itself
    friend class ToolTipManagerPrivate;
    bool eventFilter(QObject *watched, QEvent *event);

    ToolTipManagerPrivate *const d;
    Corona* m_corona;
    
    Q_PRIVATE_SLOT(d, void showToolTip())
    Q_PRIVATE_SLOT(d, void resetShownState())
    Q_PRIVATE_SLOT(d, void onWidgetDestroyed(QObject*))
    Q_PRIVATE_SLOT(d, void themeUpdated())
};

} // namespace Plasma

#endif // PLASMA_TOOL_TIP_MANAGER_H
