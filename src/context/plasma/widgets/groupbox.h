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

#ifndef PLASMA_GROUPBOX_H
#define PLASMA_GROUPBOX_H

#include <QtGui/QGraphicsProxyWidget>

class QGroupBox;

#include <plasma/plasma_export.h>

namespace Plasma
{

class GroupBoxPrivate;

/**
 * @class GroupBox plasma/widgets/groupbox.h <Plasma/Widgets/GroupBox>
 *
 * @short Provides a plasma-themed QGroupBox.
 */
class PLASMA_EXPORT GroupBox : public QGraphicsProxyWidget
{
    Q_OBJECT

    Q_PROPERTY(QGraphicsWidget *parentWidget READ parentWidget)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString styleSheet READ styleSheet WRITE setStyleSheet)
    Q_PROPERTY(QGroupBox *nativeWidget READ nativeWidget)

public:
    explicit GroupBox(QGraphicsWidget *parent = 0);
    ~GroupBox();

    /**
     * Sets the display text for this GroupBox
     *
     * @arg text the text to display; should be translated.
     */
    void setText(const QString &text);

    /**
     * @return the display text
     */
    QString text() const;

    /**
     * Sets the stylesheet used to control the visual display of this GroupBox
     *
     * @arg stylesheet a CSS string
     */
    void setStyleSheet(const QString &stylesheet);

    /**
     * @return the stylesheet currently used with this widget
     */
    QString styleSheet();

    /**
     * @return the native widget wrapped by this GroupBox
     */
    QGroupBox *nativeWidget() const;

Q_SIGNALS:

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
    GroupBoxPrivate * const d;
};

} // namespace Plasma

#endif // multiple inclusion guard
