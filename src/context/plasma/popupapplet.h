/*
 * Copyright 2008 by Montel Laurent <montel@kde.org>
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

#ifndef PLASMA_POPUPAPPLET_H
#define PLASMA_POPUPAPPLET_H

#include <plasma/applet.h>
#include <plasma/plasma_export.h>

class QGraphicsProxyWidget;
class QGraphicsLinearLayout;

namespace Plasma
{

class Dialog;
class IconWidget;
class PopupAppletPrivate;

/**
 * Allows applets to automatically 'collapse' into an icon when put in an panel, and is a convenient
 * base class for any applet that wishes to use extenders.
 *
 * Applets that subclass this class should implement either widget() or graphicsWidget() to return a
 * widget that will be displayed in the applet if the applet is in a Planar or MediaCenter form
 * factor. If the applet is put in a panel, an icon will be displayed instead, which shows the
 * widget in a popup when clicked.
 *
 * If you use this class as a base class for your extender using applet, the extender will
 * automatically be used for the popup; reimplementing graphicsWidget() is unnecessary in this case.
 */

class PLASMA_EXPORT PopupApplet : public Plasma::Applet
{
    Q_OBJECT
public:
    PopupApplet(QObject *parent, const QVariantList &args);
    ~PopupApplet();

    /**
     * @arg icon the icon that has to be displayed when the applet is in a panel.
     */
    void setPopupIcon(const QIcon &icon);

    /**
     * @arg icon the icon that has to be displayed when the applet is in a panel.
     */
    void setPopupIcon(const QString &iconName);

    /**
     * @return the icon that is displayed when the applet is in a panel.
     */
    QIcon popupIcon() const;

    /**
     * Implement either this function or graphicsWidget().
     * @return the widget that will get shown in either a layout, in the applet or in a Dialog,
     * depending on the form factor of the applet.
     */
    virtual QWidget *widget();

    /**
     * Implement either this function or widget().
     * @return the widget that will get shown in either a layout, in the applet or in a Dialog,
     * depending on the form factor of the applet.
     */
    virtual QGraphicsWidget *graphicsWidget();

    /**
     * Shows the dialog showing the widget if the applet is in a panel.
     * @arg displayTime the time in ms that the popup should be displayed, defaults to 0 which means
     * always (until the user closes it again, that is).
     */
    void showPopup(uint displayTime = 0);

    /**
     * @return the placement of the popup relating to the icon
     */
    Plasma::PopupPlacement popupPlacement() const;

    /**
     * This event handler can be reimplemented in a subclass to receive an
     * event before the popup is shown or hidden.
     * @arg show true if the popup is going to be shown, false if the popup
     * is going to be hidden.
     * Note that showing and hiding the popup on click is already done in PopupApplet.
     */
    virtual void popupEvent(bool show);

public Q_SLOTS:
    /**
     * Hides the popup.
     */
    void hidePopup();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);

private:
    Q_PRIVATE_SLOT(d, void togglePopup())
    Q_PRIVATE_SLOT(d, void hideTimedPopup())
    Q_PRIVATE_SLOT(d, void clearPopupLostFocus())
    Q_PRIVATE_SLOT(d, void dialogSizeChanged())
    Q_PRIVATE_SLOT(d, void dialogStatusChanged(bool))

    friend class Applet;
    PopupAppletPrivate * const d;
};

} // Plasma namespace

#endif /* POPUPAPPLET_H */

