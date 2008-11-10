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

#ifndef PLASMA_COMBOBOX_H
#define PLASMA_COMBOBOX_H

#include <QtGui/QGraphicsProxyWidget>

class KComboBox;

#include <plasma/plasma_export.h>

namespace Plasma
{

class ComboBoxPrivate;

/**
 * @class ComboBox plasma/widgets/combobox.h <Plasma/Widgets/ComboBox>
 *
 * @short Provides a Plasma-themed combo box.
 */
class PLASMA_EXPORT ComboBox : public QGraphicsProxyWidget
{
    Q_OBJECT

    Q_PROPERTY(QGraphicsWidget *parentWidget READ parentWidget)
    Q_PROPERTY(QString text READ text)
    Q_PROPERTY(QString styleSheet READ styleSheet WRITE setStyleSheet)
    Q_PROPERTY(KComboBox *nativeWidget READ nativeWidget)

public:
    explicit ComboBox(QGraphicsWidget *parent = 0);
    ~ComboBox();

    /**
     * @return the display text
     */
    QString text() const;

    /**
     * Sets the stylesheet used to control the visual display of this ComboBox
     *
     * @arg stylesheet a CSS string
     */
    void setStyleSheet(const QString &stylesheet);

    /**
     * @return the stylesheet currently used with this widget
     */
    QString styleSheet();

    /**
     * @return the native widget wrapped by this ComboBox
     */
    KComboBox *nativeWidget() const;

    /**
     * Adds an item to the combobox with the given text. The
     * item is appended to the list of existing items.
     */
    void addItem(const QString &text);

public Q_SLOTS:
    void clear();

Q_SIGNALS:
    void activated(const QString & text);

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event);
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget);
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

private:
    ComboBoxPrivate * const d;

    friend class ComboBoxPrivate;
    Q_PRIVATE_SLOT(d, void syncBorders())
    Q_PRIVATE_SLOT(d, void animationUpdate(qreal progress))
};

} // namespace Plasma

#endif // multiple inclusion guard
