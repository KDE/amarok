/*
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PLASMA_VIEW_H
#define PLASMA_VIEW_H

#include <QtGui/QApplication>
#include <QtGui/QGraphicsView>

#include <KDE/KConfigGroup>

#include <plasma/plasma_export.h>

namespace Plasma
{

class Containment;
class Corona;
class ViewPrivate;

/**
 * @class View plasma/view.h <Plasma/View>
 *
 * @short A QGraphicsView for a single Containment
 *
 * Each View is associated with a Plasma::Containment and tracks geometry
 * changes, maps to the current desktop (if any) among other helpful
 * utilities. It isn't stricly required to use a Plasma::View with Plasma
 * enabled applications, but it can make some things easier.
 */
class PLASMA_EXPORT View : public QGraphicsView
{
    Q_OBJECT

public:
    /**
     * Constructs a view for a given contanment. An Id is automatically
     * assigned to the View.
     *
     * @arg containment the containment to center the view on
     * @arg parent the parent object for this view
     */
    explicit View(Containment *containment, QWidget *parent = 0);

    /**
     * Constructs a view for a given contanment.
     *
     * @arg containment the containment to center the view on
     * @arg viewId the id to assign to this view
     * @arg parent the parent object for this view
     */
    View(Containment *containment, int viewId, QWidget *parent = 0);

    ~View();

    /**
     * Sets whether or not to draw the containment wallpaper when painting
     * on this item
     */
    void setWallpaperEnabled(bool draw);

    /**
     * @return whether or not containments should draw wallpaper
     */
    bool isWallpaperEnabled() const;

    /**
     * Sets which screen this view is associated with, if any.
     * This will also set the containment if a valid screen is specified
     *
     * @arg screen the xinerama screen number; -1 for no screen
     */
    void setScreen(int screen);

    /**
     * Returns the screen this view is associated with
     *
     * @return the xinerama screen number, or -1 for none
     */
    int screen() const;

    /**
     * Sets which virtual desktop this view is asociated with, if any.
     *
     * @arg desktop a valid desktop number, -1 for all desktops, less than -1 for none
     */
    void setDesktop(int desktop);

    /**
     * The virtual desktop this view is associated with
     *
     * @return the desktop number, -1 for all desktops and less than -1 for none
     */
    int desktop() const;

    /**
     * The virtual desktop this view is actually being viewed on
     *
     * @return the desktop number (always valid, never < 0)
     */
    int effectiveDesktop() const;

    /**
     * @return the containment associated with this view, or 0 if none is
     */
    Containment *containment() const;

    /**
     * Swaps one containment with another.
     *
     * @param existing the existing containment to swap out
     * @param name the plugin name for the new containment.
     * @param args argument list to pass to the containment
     */
    Containment *swapContainment(Plasma::Containment *existing,
                                 const QString &name,
                                 const QVariantList &args = QVariantList());

    /**
     * Swap the containment for this view, which will also cause the view
     * to track the geometry of the containment.
     *
     * @param name the plugin name for the new containment.
     * @param args argument list to pass to the containment
     */
    Containment *swapContainment(const QString &name,
                                 const QVariantList &args = QVariantList());

    /**
     * Set whether or not the view should adjust its size when the associated
     * containment does.
     * @arg trackChanges true to synchronize the view's size with the containment's
     * (this is the default behaviour), false to ignore containment size changes
     */
    void setTrackContainmentChanges(bool trackChanges);

    /**
     * @return whether or not the view tracks changes to the containment
     */
    bool trackContainmentChanges();

    /**
     * @param pos the position in screen coordinates.
     * @return the Plasma::View that is at position pos.
     */
    static View * topLevelViewAt(const QPoint & pos);

    /**
     * @return the id of the View set in the constructor
     */
    int id() const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever the containment being viewed has
     * changed its geometry, but before the View has shifted the viewd scene rect
     * to the new geometry. This is useful for Views which want to keep
     * their rect() in sync with the containment'sa
     */
    void sceneRectAboutToChange();

    /**
     * This signal is emitted whenever the containment being viewed has
     * changed its geometry, and after the View has shifted the viewd scene rect
     * to the new geometry. This is useful for Views which want to keep
     * their rect() in sync with the containment's.
     */
    void sceneRectChanged();

public Q_SLOTS:
    /**
     * Sets the containment for this view, which will also cause the view
     * to track the geometry of the containment.
     *
     * @arg containment the containment to center the view on
     */
    virtual void setContainment(Plasma::Containment *containment);

protected:
    /**
     * @return a KConfigGroup in the application's config file unique to the view
     */
    KConfigGroup config() const;

private:
    ViewPrivate * const d;

    Q_PRIVATE_SLOT(d, void updateSceneRect())
    Q_PRIVATE_SLOT(d, void containmentDestroyed())

    friend class ViewPrivate;
};

} // namespace Plasma

#endif

