/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_PLAYLISTHEADER_H
#define AMAROK_PLAYLISTHEADER_H

#include <QMap>
#include <QStringList>
#include <QWidget>

class QHBoxLayout;
class QVBoxLayout;
class QLabel;

namespace Playlist
{
class HeaderWidget : public QWidget
{
    Q_OBJECT
    static const QString HeaderMimeType;
public:
    HeaderWidget( QWidget* parent );
protected:
    void enterEvent( QEvent* event );
    void leaveEvent( QEvent* event );
    void dragEnterEvent( QDragEnterEvent *event );
    void dropEvent( QDropEvent *event );
    void mousePressEvent( QMouseEvent *event );

private:
    QHBoxLayout* m_topLayout;

    QList<QVBoxLayout*> m_verticalLayouts;
    QList<QLabel*> m_labels;
    QMap<QString, QLabel*> m_textToLabel;
    QStringList m_test;
};
}
#endif
