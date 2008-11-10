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

#ifndef PLASMA_LINEEDIT_H
#define PLASMA_LINEEDIT_H

#include <QtGui/QGraphicsProxyWidget>

class KLineEdit;

#include <plasma/plasma_export.h>

namespace Plasma
{

class LineEditPrivate;

/**
 * @class LineEdit plasma/widgets/lineedit.h <Plasma/Widgets/LineEdit>
 *
 * @short Provides a plasma-themed KLineEdit.
 */
class PLASMA_EXPORT LineEdit : public QGraphicsProxyWidget
{
    Q_OBJECT

    Q_PROPERTY(QGraphicsWidget *parentWidget READ parentWidget)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString styleSheet READ styleSheet WRITE setStyleSheet)
    Q_PROPERTY(KLineEdit *nativeWidget READ nativeWidget)

public:
    explicit LineEdit(QGraphicsWidget *parent = 0);
    ~LineEdit();

    /**
     * Sets the display text for this LineEdit
     *
     * @arg text the text to display; should be translated.
     */
    void setText(const QString &text);

    /**
     * @return the display text
     */
    QString text() const;

    /**
     * Sets the stylesheet used to control the visual display of this LineEdit
     *
     * @arg stylesheet a CSS string
     */
    void setStyleSheet(const QString &stylesheet);

    /**
     * @return the stylesheet currently used with this widget
     */
    QString styleSheet();

    /**
     * @return the native widget wrapped by this LineEdit
     */
    KLineEdit *nativeWidget() const;

Q_SIGNALS:
    void editingFinished();
    void returnPressed();
    void textEdited(const QString &text);

private:
    LineEditPrivate *const d;
};

} // namespace Plasma

#endif // multiple inclusion guard
