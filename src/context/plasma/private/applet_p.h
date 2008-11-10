 /*
 *   Copyright 2005 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2007 by Riccardo Iaconelli <riccardo@kde.org>
 *   Copyright 2008 by Ménard Alexis <darktears31@gmail.com>
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

#ifndef PLASMA_APPLET_P_H
#define PLASMA_APPLET_P_H

#include <KActionCollection>

namespace Plasma
{

class FrameSvg;
class AppletScript;
class Wallpaper;

class AppletOverlayWidget : public QGraphicsWidget
{
public:
    AppletOverlayWidget(QGraphicsWidget *parent);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
};

class AppletPrivate
{
public:
    AppletPrivate(KService::Ptr service, int uniqueID, Applet *applet);
    ~AppletPrivate();

    void init();

    // put all setup routines for script here. at this point we can assume that
    // package exists and that we have a script engin
    void setupScriptSupport();

    /**
     * Sets whether or not this Applet is acting as a Containment
     */
    void setIsContainment(bool isContainment);

    QString globalName() const;
    QString instanceName();
    void scheduleConstraintsUpdate(Plasma::Constraints c);
    KConfigGroup *mainConfigGroup();
    QString visibleFailureText(const QString &reason);
    void checkImmutability();
    void themeChanged();
    void resetConfigurationObject();
    void appletAnimationComplete(QGraphicsItem *item, Plasma::Animator::Animation anim);
    void selectItemToDestroy();
    void updateRect(const QRectF &rect);
    void setFocus();
    void cleanUpAndDelete();

    static uint s_maxAppletId;
    static uint s_maxZValue;
    static uint s_minZValue;
    static PackageStructure::Ptr packageStructure;

    //TODO: examine the usage of memory here; there's a pretty large
    //      number of members at this point.
    uint appletId;
    Applet *q;

    Extender *extender;
    Applet::BackgroundHints backgroundHints;
    KPluginInfo appletDescription;
    AppletOverlayWidget *needsConfigOverlay;
    QList<QGraphicsItem*> registeredAsDragHandle;
    QStringList loadedEngines;
    Plasma::FrameSvg *background;
    AppletScript *script;
    Package *package;
    ConfigLoader *configLoader;
    KConfigGroup *mainConfig;
    Plasma::Constraints pendingConstraints;
    Plasma::AspectRatioMode aspectRatioMode;
    ImmutabilityType immutability;
    KActionCollection actions;
    KAction *activationAction;
    int constraintsTimerId;
    int modificationsTimerId;
    bool hasConfigurationInterface : 1;
    bool failed : 1;
    bool isContainment : 1;
    bool square : 1;
    bool transient : 1;
    bool ghost : 1;
};

} // Plasma namespace

#endif
