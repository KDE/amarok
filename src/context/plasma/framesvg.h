/*
 *   Copyright 2008 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 Marco Martin <notmart@gmail.com>
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

#ifndef PLASMA_FRAMESVG_H
#define PLASMA_FRAMESVG_H

#include <QtCore/QObject>
#include <QtGui/QPixmap>

#include <plasma/plasma_export.h>

#include <plasma/plasma.h>
#include <plasma/svg.h>

class QPainter;
class QPoint;
class QPointF;
class QRect;
class QRectF;
class QSize;
class QSizeF;
class QMatrix;

namespace Plasma
{

class FrameSvgPrivate;

/**
 * @class FrameSvg plasma/framesvg.h <Plasma/FrameSvg>
 *
 * @short Provides an SVG with borders.
 *
 * When using SVG images for a background of an object that may change
 * its aspect ratio, such as a dialog, simply scaling a single image
 * may not be enough.
 *
 * FrameSvg allows SVGs to provide several elements for borders as well
 * as a central element, each of which are scaled individually.  These
 * elements should be named
 *
 *  - @c center  - the central element, which will be scaled in both directions
 *  - @c top     - the top border; the height is fixed, but it will be scaled
 *                 horizontally to the same width as @c center
 *  - @c bottom  - the bottom border; scaled in the same way as @c top
 *  - @c left    - the left border; the width is fixed, but it will be scaled
 *                 vertically to the same height as @c center
 *  - @c right   - the right border; scaled in the same way as @c left
 *  - @c topleft - fixed size; must be the same height as @c top and the same
 *                 width as @c left
 *  - @c bottomleft, @c topright, @c bottomright - similar to @c topleft
 *
 * @c center must exist, but all the others are optional.  @c topleft and
 * @c topright will be ignored if @c top does not exist, and similarly for
 * @c bottomleft and @c bottomright.
 *
 * @see Plamsa::Svg
 **/
class PLASMA_EXPORT FrameSvg : public Svg
{
    Q_OBJECT
    public:
        /**
         * These flags represents what borders should be drawn
         */
        enum EnabledBorder {
            NoBorder = 0,
            TopBorder = 1,
            BottomBorder = 2,
            LeftBorder = 4,
            RightBorder = 8,
            AllBorders = TopBorder | BottomBorder | LeftBorder | RightBorder
        };
        Q_DECLARE_FLAGS(EnabledBorders, EnabledBorder)

        /**
         * Constructs a new FrameSvg that paints the proper named subelements
         * as borders. It may also be used as a regular Plasma::Svg object
         * for direct access to elements in the Svg.
         *
         * @arg parent options QObject to parent this to
         *
         * @related Plasma::Theme
         */
        explicit FrameSvg(QObject *parent = 0);
        ~FrameSvg();

        /**
         * Loads a new Svg
         * @arg imagePath the new file
         */
        void setImagePath(const QString &path);

        /**
         * Sets what borders should be painted
         * @arg flags borders we want to paint
         */
        void setEnabledBorders(const EnabledBorders borders);

        /**
         * Convenience method to get the enabled borders
         * @return what borders are painted
         */
        EnabledBorders enabledBorders() const;

        /**
         * Resize the frame maintaining the same border size
         * @arg size the new size of the frame
         */
        void resizeFrame(const QSizeF &size);

        /**
         * @returns the size of the frame
         */
        QSizeF frameSize() const;

        /**
         * Returns the margin size given the margin edge we want
         * @arg edge the margin edge we want, top, bottom, left or right
         * @return the margin size
         */
        qreal marginSize(const Plasma::MarginEdge edge) const;

        /**
         * Convenience method that extracts the size of the four margins
         * in the four output parameters
         * @arg left left margin size
         * @arg top top margin size
         * @arg right right margin size
         * @arg bottom bottom margin size
         */
        void getMargins(qreal &left, qreal &top, qreal &right, qreal &bottom) const;

        /**
         * @return the rectangle of the center element, taking the margins into account.
         */
        QRectF contentsRect() const;

        /**
         * Sets the prefix (@see setElementPrefix) to 'north', 'south', 'west' and 'east'
         * when the location is TopEdge, BottomEdge, LeftEdge and RightEdge,
         * respectively. Clears the prefix in other cases.
         * @arg location location
         */
        void setElementPrefix(Plasma::Location location);

        /**
         * Sets the prefix for the SVG elements to be used for painting. For example,
         * if prefix is 'active', then instead of using the 'top' element of the SVG
         * file to paint the top border, 'active-top' element will be used. The same
         * goes for other SVG elements.
         *
         * If the elements with prefixes are not present, the default ones are used.
         * (for the sake of speed, the test is present only for the 'center' element)
         *
         * Setting the prefix manually resets the location to Floating.
         * If the
         * @arg prefix prefix for the SVG element names
         */
        void setElementPrefix(const QString & prefix);

        /**
         * @return true if the svg has the necessary elements with the given prefix
         * to draw a frame
         * @arg prefix the given prefix we want to check if drawable
         */
        bool hasElementPrefix(const QString & prefix) const;

        /**
         * This is an overloaded method provided for convenience equivalent to
         * hasElementPrefix("north"), hasElementPrefix("south")
         * hasElementPrefix("west") and hasElementPrefix("east")
         * @return true if the svg has the necessary elements with the given prefix
         * to draw a frame.
         * @arg location the given prefix we want to check if drawable
         */
        bool hasElementPrefix(Plasma::Location location) const;

        /**
         * Returns the prefix for SVG elements of the FrameSvg
         * @return the prefix
         */
        QString prefix();

        /**
         * Returns a monochrome mask that tightly contains the fully opaque areas of the svg
         * @return a monochrome bitmap of opaque areas
         */
        QBitmap mask() const;

       /**
        * Sets whether saving all the rendered prefixes in a cache or not
        * @arg cache if use the cache or not
        */
        void setCacheAllRenderedFrames(bool cache);

       /**
        * @return if all the different prefixes should be kept in a cache when rendered
        */
        bool cacheAllRenderedFrames() const;

       /**
        * Deletes the internal cache freeing memory: use this if you want to switch the rendered
        * element and you don't plan to switch back to the previous one for a long time and you
        * used setUseCache(true)
        */
        void clearCache();

       /**
         * Returns a pixmap of the SVG represented by this object.
         *
         * @arg elelementId the ID string of the element to render, or an empty
         *                  string for the whole SVG (the default)
         * @return a QPixmap of the rendered SVG
         */
        Q_INVOKABLE QPixmap framePixmap();

        /**
         * Paints the loaded SVG with the elements that represents the border
         * @arg painter the QPainter to use
         * @arg target the target rectangle on the paint device
         * @arg source the portion rectangle of the source image
         */
        Q_INVOKABLE void paintFrame(QPainter *painter, const QRectF &target,
                                    const QRectF &source = QRectF());

        /**
         * Paints the loaded SVG with the elements that represents the border
         * This is an overloaded member provided for convenience
         * @arg painter the QPainter to use
         * @arg pos where to paint the svg
         */
        Q_INVOKABLE void paintFrame(QPainter *painter, const QPointF &pos = QPointF(0, 0));

    private:
        FrameSvgPrivate *const d;

        Q_PRIVATE_SLOT(d, void updateSizes())
        Q_PRIVATE_SLOT(d, void updateNeeded())
};

} // Plasma namespace

Q_DECLARE_OPERATORS_FOR_FLAGS(Plasma::FrameSvg::EnabledBorders)

#endif // multiple inclusion guard
