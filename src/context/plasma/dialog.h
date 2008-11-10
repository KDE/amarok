/*
 * Copyright 2008 by Alessandro Diaferia <alediaferia@gmail.com>
 * Copyright 2007 by Alexis Ménard <darktears31@gmail.com>
 * Copyright 2007 Sebastian Kuegler <sebas@kde.org>
 * Copyright 2006 Aaron Seigo <aseigo@kde.org>
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

#ifndef PLASMA_DIALOG_H
#define PLASMA_DIALOG_H

#include <QtGui/QWidget>
#include <QtGui/QGraphicsSceneEvent>
#include <QtGui/QGraphicsView>

#include <plasma/plasma_export.h>

namespace Plasma
{

class DialogPrivate;

/**
 * @class Dialog plasma/dialog.h <Plasma/Dialog>
 *
 * @short A dialog that uses the Plasma style
 *
 * Dialog provides a dialog-like widget that can be used to display additional
 * information.
 *
 * Dialog uses the plasma theme, and usually has no window decoration. It's meant
 * as an interim solution to display widgets as extension to plasma applets, for
 * example when you click on an applet like the devicenotifier or the clock, the
 * widget that is then displayed, is a Dialog.
 */
class PLASMA_EXPORT Dialog : public QWidget
{
    Q_OBJECT

    public:
        /**
         * Use these flags to choose the active resize corners.
         */
        enum ResizeCorner {
            NoCorner = 0,
            NorthEast = 1,
            SouthEast = 2,
            NorthWest = 4,
            SouthWest = 8,
            All = NorthEast | SouthEast | NorthWest | SouthWest
        };
        Q_DECLARE_FLAGS(ResizeCorners, ResizeCorner)

        /**
         * @arg parent the parent widget, for plasmoids, this is usually 0.
         * @arg f the Qt::WindowFlags, default is to not show a windowborder.
         */
        explicit Dialog(QWidget * parent = 0, Qt::WindowFlags f =  Qt::Window);
        virtual ~Dialog();

        void setGraphicsWidget(QGraphicsWidget *widget);
        QGraphicsWidget *graphicsWidget();

        /**
         * @arg corners the corners the resize handlers should be placed in.
         */
        void setResizeHandleCorners(ResizeCorners corners);

        /**
         * Convenience method to get the enabled resize corners.
         * @return which resize corners are active.
         */
        ResizeCorners resizeCorners() const;

    Q_SIGNALS:
        /**
         * Fires when the dialog automatically resizes.
         */
        void dialogResized();

        /**
         * Emit a signal when the dialog become visible/invisible
         */
        void dialogVisible(bool status);

    protected:
        /**
         * Reimplemented from QWidget
         */
        void paintEvent(QPaintEvent *e);
        void resizeEvent(QResizeEvent *e);
        bool eventFilter(QObject *watched, QEvent *event);
        void hideEvent (QHideEvent *event);
        void showEvent (QShowEvent *event);
        void mouseMoveEvent (QMouseEvent *event);
        void mousePressEvent (QMouseEvent *event);
        void mouseReleaseEvent (QMouseEvent *event);

        /**
         * Convenience method to know whether the point is in a control area (e.g. resize area)
         * or not.
         * @return true if the point is in the control area.
         */
        bool inControlArea(const QPoint &point);

    private:
        DialogPrivate *const d;

        friend class DialogPrivate;
        /**
         * React to theme changes
         */
        Q_PRIVATE_SLOT(d, void themeUpdated())
};

} // Plasma namespace

Q_DECLARE_OPERATORS_FOR_FLAGS(Plasma::Dialog::ResizeCorners)

#endif
