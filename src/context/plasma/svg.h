/*
 *   Copyright 2006-2007 Aaron Seigo <aseigo@kde.org>
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

#ifndef PLASMA_SVG_H
#define PLASMA_SVG_H

#include <QtCore/QObject>
#include <QtGui/QPixmap>

#include <plasma/plasma_export.h>

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

class SvgPrivate;
class FrameSvgPrivate;

/**
 * @class Svg plasma/svg.h <Plasma/Svg>
 *
 * @short A theme aware image-centric SVG class
 *
 * Plasma::Svg provides a class for rendering SVG images to a QPainter in a
 * convenient manner. Unless an absolute path to a file is provided, it loads
 * the SVG document using Plasma::Theme. It also provides a number of internal
 * optimizations to help lower the cost of painting SVGs, such as caching.
 *
 * @see Plasma::FrameSvg
 **/
class PLASMA_EXPORT Svg : public QObject
{
    Q_OBJECT
    Q_ENUMS(ContentType)
    Q_PROPERTY(QSize size READ size)
    Q_PROPERTY(bool multipleImages READ containsMultipleImages WRITE setContainsMultipleImages)
    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath)

    public:

        /**
         * Constructs an SVG object that implicitly shares and caches rendering
         * As opposed to QSvgRenderer, which this class uses internally,
         * Plasma::Svg represents an image generated from an SVG. As such, it
         * has a related size and transform matrix (the latter being provided
         * by the painter used to paint the image).
         *
         * The size is initialized to be the SVG's native size.
         *
         * @arg parent options QObject to parent this to
         *
         * @related Plasma::Theme
         */
        explicit Svg(QObject *parent = 0);
        ~Svg();

        /**
         * Returns a pixmap of the SVG represented by this object.
         *
         * @arg elelementId the ID string of the element to render, or an empty
         *                  string for the whole SVG (the default)
         * @return a QPixmap of the rendered SVG
         */
        Q_INVOKABLE QPixmap pixmap(const QString &elementID = QString());

        /**
         * Paints the SVG represented by this object
         * @arg painter the QPainter to use
         * @arg point the position to start drawing; the entire svg will be
         *      drawn starting at this point.
         * @arg elelementId the ID string of the element to render, or an empty
         *                  string for the whole SVG (the default)
         */
        Q_INVOKABLE void paint(QPainter *painter, const QPointF &point,
                               const QString &elementID = QString());

        /**
         * Paints the SVG represented by this object
         * @arg painter the QPainter to use
         * @arg x the horizontal coordinate to start painting from
         * @arg y the vertical coordinate to start painting from
         * @arg elelementId the ID string of the element to render, or an empty
         *                  string for the whole SVG (the default)
         */
        Q_INVOKABLE void paint(QPainter *painter, int x, int y,
                               const QString &elementID = QString());

        /**
         * Paints the SVG represented by this object
         * @arg painter the QPainter to use
         * @arg rect the rect to draw into; if smaller than the current size
         *           the drawing is starting at this point.
         * @arg elelementId the ID string of the element to render, or an empty
         *                  string for the whole SVG (the default)
         */
        Q_INVOKABLE void paint(QPainter *painter, const QRectF &rect,
                               const QString &elementID = QString());

        /**
         * Paints the SVG represented by this object
         * @arg painter the QPainter to use
         * @arg x the horizontal coordinate to start painting from
         * @arg y the vertical coordinate to start painting from
         * @arg width the width of the element to draw
         * @arg height the height of the element do draw
         * @arg elelementId the ID string of the element to render, or an empty
         *                  string for the whole SVG (the default)
         */
        Q_INVOKABLE void paint(QPainter *painter, int x, int y, int width,
                               int height, const QString &elementID = QString());

        /**
         * Currently set size of the SVG
         * @return the current size of a given element
         **/
        QSize size() const;

        /**
         * Resizes the rendered image. Rendering will actually take place on
         * the next call to paint.
         * @arg width the new width
         * @arg height the new height
         **/
        Q_INVOKABLE void resize(qreal width, qreal height);

        /**
         * Resizes the rendered image. Rendering will actually take place on
         * the next call to paint.
         * @arg size the new size of the image
         **/
        Q_INVOKABLE void resize(const QSizeF &size);

        /**
         * Resizes the rendered image to the natural size of the SVG.
         * Rendering will actually take place on the next call to paint.
         **/
        Q_INVOKABLE void resize();

        /**
         * Size of a given element
         * @arg elementId the id of the element to check
         * @return the current size of a given element, given the current size of the Svg
         **/
        Q_INVOKABLE QSize elementSize(const QString &elementId) const;

        /**
         * The bounding rect of a given element
         * @arg elementId the id of the element to check
         * @return the current rect of a given element, given the current size of the Svg
         **/
        Q_INVOKABLE QRectF elementRect(const QString &elementId) const;

        /**
         * Check when an element exists in the loaded Svg
         * @arg elementId the id of the element to check
         * @return true if the element is defined in the Svg, otherwise false
         **/
        Q_INVOKABLE bool hasElement(const QString &elementId) const;

        /**
         * Returns the element (by id) at the given point. An empty string is
         * returned if no element is at that point.
         */
        Q_INVOKABLE QString elementAtPoint(const QPoint &point) const;

        /**
         * @return true if the SVG file exists and the document is valid,
         *         otherwise false. This method can be expensive as it
         *         causes disk access.
         **/
        Q_INVOKABLE bool isValid() const;

       /**
        * Set if the svg contains a single image or multiple ones.
        * @arg multiple true if the svg contains multiple images
        */
        void setContainsMultipleImages(bool multiple);

       /**
        * @return whether or not the svg contains multiple images or not
        */
        bool containsMultipleImages() const;

        /**
         * Convenience method for setting the svg file to use for the Svg.
         * @arg svgFilePath the filepath including name of the svg.
         */
        void setImagePath(const QString &svgFilePath);

        /**
         * Convenience method to get the svg filepath and name of svg.
         * @return the svg's filepath including name of the svg.
         */
        QString imagePath() const;

    Q_SIGNALS:
        void repaintNeeded();

    private:
        SvgPrivate *const d;

        Q_PRIVATE_SLOT(d, void themeChanged())
        Q_PRIVATE_SLOT(d, void colorsChanged())

        friend class SvgPrivate;
        friend class FrameSvgPrivate;
};

} // Plasma namespace

#endif // multiple inclusion guard

