/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
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

#ifndef PLASMA_PUSHBUTTON_H
#define PLASMA_PUSHBUTTON_H

#include <QtGui/QGraphicsProxyWidget>

class KPushButton;

#include <plasma/plasma_export.h>

namespace Plasma
{

class PushButtonPrivate;

/**
 * @class PushButton plasma/widgets/pushbutton.h <Plasma/Widgets/PushButton>
 *
 * @short Provides a plasma-themed KPushButton.
 */
class PLASMA_EXPORT PushButton : public QGraphicsProxyWidget
{
    Q_OBJECT

    Q_PROPERTY(QGraphicsWidget *parentWidget READ parentWidget)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString image READ image WRITE setImage)
    Q_PROPERTY(QString stylesheet READ styleSheet WRITE setStyleSheet)
    Q_PROPERTY(KPushButton *nativeWidget READ nativeWidget)

public:
    explicit PushButton(QGraphicsWidget *parent = 0);
    ~PushButton();

    /**
     * Sets the display text for this PushButton
     *
     * @arg text the text to display; should be translated.
     */
    void setText(const QString &text);

    /**
     * @return the display text
     */
    QString text() const;

    /**
     * Sets the path to an image to display.
     *
     * @arg path the path to the image; if a relative path, then a themed image will be loaded.
     */
    void setImage(const QString &path);

    /**
     * @return the image path being displayed currently, or an empty string if none.
     */
    QString image() const;

    /**
     * Sets the stylesheet used to control the visual display of this PushButton
     *
     * @arg stylesheet a CSS string
     */
    void setStyleSheet(const QString &stylesheet);

    /**
     * @return the stylesheet currently used with this widget
     */
    QString styleSheet();

    /**
     * @return the native widget wrapped by this PushButton
     */
    KPushButton *nativeWidget() const;

Q_SIGNALS:
    void clicked();

protected:
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = 0);
    void resizeEvent(QGraphicsSceneResizeEvent *event);
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

private:
    PushButtonPrivate *const d;

    friend class PushButtonPrivate;
    Q_PRIVATE_SLOT(d, void syncBorders())
    Q_PRIVATE_SLOT(d, void animationUpdate(qreal progress))
};

} // namespace Plasma

#endif // multiple inclusion guard
