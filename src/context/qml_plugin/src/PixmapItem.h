/*
 * Copyright 2018  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PIXMAPITEM_H
#define PIXMAPITEM_H

#include <QPixmap>
#include <QQuickItem>
#include <QSGTexture>


class PixmapItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY( QPixmap source READ source WRITE setSource NOTIFY sourceChanged RESET resetSource )
    Q_PROPERTY( bool valid READ valid NOTIFY sourceChanged )

public:
    PixmapItem();

    void setSource( const QPixmap &source );
    void resetSource() { setSource( QPixmap() ); }
    QPixmap source() const { return m_source; }
    bool valid() const { return !m_source.isNull(); }

    QSGNode* updatePaintNode( QSGNode * oldNode, UpdatePaintNodeData * updatePaintNodeData ) override;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void geometryChanged( const QRectF &newGeometry, const QRectF &oldGeometry ) override;
#else
    void geometryChange( const QRectF &newGeometry, const QRectF &oldGeometry ) override;
#endif

Q_SIGNALS:
    void sourceChanged();

private:
    QPixmap m_source;
    bool m_pixmapChanged;
    bool m_sizeChanged;
    QSharedPointer<QSGTexture> m_texture;
};

#endif // PIXMAPITEM_H
