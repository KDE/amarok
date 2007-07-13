/***************************************************************************
 *   Copyright (C) 2007 by Mark Kretschmann <kretschmann@kde.org>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef AMAROK_COVERBLING_H
#define AMAROK_COVERBLING_H

#include <QGLWidget>
#include <QThread>


class CoverBling : public QGLWidget
{
    Q_OBJECT

    public:
        CoverBling( QWidget* parent );

    protected:
        void initializeGL();
        void resizeGL( int width, int height );
        void paintGL();

    private:
        GLuint m_textureId;
        GLuint m_texturedRectList;
        GLuint m_texturedRectReflectedList;
        float m_xOffset;
        float m_zOffset;
        int m_textureWidth;
        int m_textureHeight;
        float m_aspectX;
        float m_aspectY;
};


class UpdateThread : public QThread
{
    Q_OBJECT

    public:
        UpdateThread( QObject* parent );

    protected:
        void run();
};


#endif /* AMAROK_COVERBLING_H */


