/*
 * Copyright (c) 2003-2005 Max Howell <max.howell@methylblue.com>
 * Copyright (c) 2005-2013 Mark Kretschmann <kretschmann@kde.org>
 * Copyright 2017 Malte Veerman <malte.veerman@gmail.com>
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
 *
 */

#ifndef BLOCKRENDERER_H
#define BLOCKRENDERER_H

#include "BlockAnalyzer.h"
#include "BlockWorker.h"
#include "core/support/Debug.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QQuickFramebufferObject>
#include <QSGTexture>
#include <QSharedPointer>

class BlockRenderer : public QQuickFramebufferObject::Renderer
{
public:
    static const int BLOCK_HEIGHT = BlockAnalyzer::BLOCK_HEIGHT;

    BlockRenderer() {}

protected:
    QOpenGLFramebufferObject* createFramebufferObject(const QSize &size) override
    {
        QOpenGLFramebufferObject* fo = new QOpenGLFramebufferObject(size);
        fo->setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        return fo;
    }

    void render() override
    {
        QOpenGLPaintDevice d;
        d.setSize(framebufferObject()->size());
        QPainter p(&d);

        // Draw the background
        p.drawPixmap(QRect(QPoint(0, 0), framebufferObject()->size()), m_backgroundPixmap);


        const int frameHeight = framebufferObject()->height();

        for( uint x = 0; x < (uint)m_store.size(); ++x )
        {
            // Draw fade bars
            for( const auto &fadebar : m_fadebars.at(x) )
            {
                if( fadebar.intensity > 0 )
                {
                    const uint offset = fadebar.intensity;
                    const int fadeHeight = frameHeight - fadebar.y * (BLOCK_HEIGHT + 1);
                    if( fadeHeight > 0 )
                        p.drawPixmap(x * ( m_columnWidth + 1 ), 0, m_fadeBarsPixmaps.value(offset), 0, 0, m_columnWidth, fadeHeight);
                }
            }

            // Draw bars
            const int height = frameHeight - m_store.at(x) * (BLOCK_HEIGHT + 1);
            if (height > 0)
                p.drawPixmap(x * (m_columnWidth + 1), 0, m_barPixmap, 0, 0, m_columnWidth, height);

            // Draw top bar
            p.drawPixmap(x * (m_columnWidth + 1), height + BLOCK_HEIGHT - 1, m_topBarPixmap);
        }
    }

    void synchronize(QQuickFramebufferObject *item) override
    {
        auto analyzer = qobject_cast<BlockAnalyzer*>(item);
        if (!analyzer)
            return;

        m_rows = analyzer->m_rows;
        m_columnWidth = analyzer->m_columnWidth;

        auto worker = qobject_cast<const BlockWorker*>(analyzer->worker());
        if (worker)
        {
            worker->m_mutex.lock();
            m_store = worker->m_store;
            m_fadebars = worker->m_fadebars;
            worker->m_mutex.unlock();
        }

        if (analyzer->m_pixmapsChanged)
        {
            m_barPixmap = analyzer->m_barPixmap;
            m_topBarPixmap = analyzer->m_topBarPixmap;
            m_backgroundPixmap = analyzer->m_backgroundPixmap;
            m_fadeBarsPixmaps = analyzer->m_fadeBarsPixmaps;

            analyzer->m_pixmapsChanged = false;
        }
    }

private:
    QVector<double> m_store;
    QVector<QList<BlockWorker::Fadebar> > m_fadebars;
    int m_rows;
    int m_columnWidth;

    QPixmap m_barPixmap;
    QPixmap m_topBarPixmap;
    QPixmap m_backgroundPixmap;
    QVector<QPixmap> m_fadeBarsPixmaps;
};

#endif //BLOCKRENDERER_H
