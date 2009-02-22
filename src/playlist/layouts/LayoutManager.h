/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2009  Seb Ruiz <ruiz@kde.org>                           *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#ifndef PLAYLISTLAYOUTMANAGER_H
#define PLAYLISTLAYOUTMANAGER_H

#include "PrettyItemConfig.h"

#include <QStringList>
#include <QString>
#include <QMap>

class QDomElement;
class QDomDocument;

namespace Playlist {

/**
    Class for keeping track of playlist layouts and loading/saving them to xml files
    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class LayoutManager : public QObject
{
    Q_OBJECT

public:
    static LayoutManager* instance();

    QStringList layouts() const;
    void setActiveLayout( const QString &layout );
    void setPreviewLayout( const PlaylistLayout &layout );
    PlaylistLayout layout( const QString &layout ) const;
    PlaylistLayout activeLayout() const;
    QString activeLayoutName() const;

    bool isDefaultLayout( const QString &layout ) const;

    void addUserLayout( const QString &name, PlaylistLayout layout );
    void deleteLayout( const QString &layout );
    bool isDeleteable( const QString &layout ) const;

signals:
    void activeLayoutChanged();
    void layoutListChanged();

private:
    LayoutManager();

    static LayoutManager *s_instance;

    void loadDefaultLayouts();
    void loadUserLayouts();

    void loadLayouts( const QString &fileName, bool user );

    QDomElement createItemElement( QDomDocument doc, const QString &name,  const PrettyItemConfig &item ) const;
    
    PrettyItemConfig parseItemConfig( const QDomElement &elem ) const;

    QMap<QString, PlaylistLayout> m_layouts;
    QString                       m_activeLayout;
    PlaylistLayout                m_previewLayout;
};

}

#endif
