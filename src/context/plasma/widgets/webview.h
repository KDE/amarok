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

#ifndef PLASMA_WEBVIEW_H
#define PLASMA_WEBVIEW_H

#include <plasma/plasma_export.h>
#include <QtCore/QUrl>
#include <QtGui/QGraphicsWidget>

class QWebPage;
class QWebFrame;
class QKeyEvent;
class QGraphicsSceneDragDropEvent;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneWheelEvent;
class QRect;

namespace Plasma
{

class WebViewPrivate;

/**
 * @class WebView plasma/widgets/webcontent.h <Plasma/Widgets/WebView>
 *
 * @short Provides a widget to display html content in Plasma.
 */
class PLASMA_EXPORT WebView : public QGraphicsWidget
{
    Q_OBJECT

    public:
        explicit WebView(QGraphicsItem *parent = 0);
        ~WebView();

        /**
         * Sets the URL to display. Loading may happen asynchronously.
         *
         * @param url the location of the content to load.
         */
        void setUrl(const QUrl &url);

        /**
         * Sets the html to be shown along with a base URL to be used
         * to resolve relative references.
         *
         * @param html the html (in utf8) to display in the content area
         * @param baseUrl the base url for relative references
         */
        void setHtml(const QByteArray &html, const QUrl &baseUrl = QUrl());

        /**
         * Sets the html to be shown along with a base URL to be used
         * to resolve relative references.
         *
         * @param html the html (in utf8) to display in the content area
         * @param baseUrl the base url for relative references
         */
        void setHtml(const QString &html, const QUrl &baseUrl = QUrl());

        /**
         * Reimplementation
         */
        QRectF geometry() const;

        /**
         * Sets the page to use in this item. The owner of the webpage remains,
         * however if this WebView object is the owner of the current page,
         * then the current page is deleted
         *
         * @param page the page to set in this view
         */
        void setPage(QWebPage *page);

        /**
         * The QWebPage associated with this item. Useful when more
         * of the features of the full QWebPage object need to be accessed.
         */
        QWebPage *page() const;

        /**
         * The main web frame associated with this item.
         */
        QWebFrame *mainFrame() const;

        /**
         * Reimplementation
         */
        void setGeometry(const QRectF &geometry);

    Q_SIGNALS:
        /**
         * During loading progress, this signal is emitted. The values
         * are always between 0 and 100, inclusive.
         *
         * @param percent the estimated amount the loading is complete
         */
        void loadProgress(int percent);

        /**
         * This signal is emitted when loading is completed.
         *
         * @param success true if the content was loaded successfully,
         *                otherwise false
         */
        void loadFinished(bool success);

    protected:
        /**
         * Reimplementation
         */
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
        void mousePressEvent(QGraphicsSceneMouseEvent *event);
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
        void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
        void wheelEvent(QGraphicsSceneWheelEvent *event);
        void keyPressEvent(QKeyEvent * event);
        void keyReleaseEvent(QKeyEvent * event);
        void focusInEvent(QFocusEvent * event);
        void focusOutEvent(QFocusEvent * event);
        void dragEnterEvent(QGraphicsSceneDragDropEvent * event);
        void dragLeaveEvent(QGraphicsSceneDragDropEvent * event);
        void dragMoveEvent(QGraphicsSceneDragDropEvent * event);
        void dropEvent(QGraphicsSceneDragDropEvent * event);

    private:
        Q_PRIVATE_SLOT(d, void loadingFinished(bool success))
        Q_PRIVATE_SLOT(d, void updateRequested(const QRect& dirtyRect))
        Q_PRIVATE_SLOT(d, void scrollRequested(int dx, int dy, const QRect &scrollRect))

        WebViewPrivate * const d;
        friend class WebViewPrivate;
};

} // namespace Plasma

#endif // Multiple incluson guard

