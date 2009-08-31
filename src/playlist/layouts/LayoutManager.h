/****************************************************************************************
 * Copyright (c) 2008-2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef PLAYLISTLAYOUTMANAGER_H
#define PLAYLISTLAYOUTMANAGER_H

#include "LayoutItemConfig.h"

#include <QStringList>
#include <QString>
#include <QMap>

class QDomElement;
class QDomDocument;

namespace Playlist {

/**
 * Class for keeping track of playlist layouts and loading/saving them to xml files. Also keeps track
 * Of the order in which these layouts are shown to the user and makes this persistant between sessions.
 * @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 */
class LayoutManager : public QObject
{
    Q_OBJECT

public:
    /**
     * Get the class instance (Singleton Pattern).
     * @return The class instance.
     */
    static LayoutManager* instance();

    /**
     * Get the ordered list of layout names.
     * @return The list of layout names.
     */
    QStringList layouts() const;
    
    /**
     * Set the layout that is to be made active and used in the playlist.
     * @param layout The name of the layout.
     */
    void setActiveLayout( const QString &layout );
    
    /**
     * Preview a layout in the playlist. This layout does not need to be stored or have a name yet.
     * @param layout The layout to preview.
     */
    void setPreviewLayout( const PlaylistLayout &layout );
    
    /**
     * Get the layout with a specific name. Returns an empty layout if there is no layout matchig the name.
     * @param layout The name of the layout.
     * @return The layout matching the name.
     */
    PlaylistLayout layout( const QString &layout ) const;
    
    /**
     * Get the currently active layout.
     * @return the layout that is currently active.
     */
    PlaylistLayout activeLayout() const;
    
    /**
     * Get the name o the currently active layout.
     * @return The name of the layout that is currently active.
     */
    QString activeLayoutName() const;

    /**
     * Check if a named layout if one of Amaroks defaults or is one added by a user, This is important
     * as default layouts cannot be changed or deleted.
     * @param layout The name of the layout.
     * @return Is layout one of the defaults.
     */
    bool isDefaultLayout( const QString &layout ) const;

    /**
     * Add and store a new user defined layout.
     * @param name The name of the new layout.
     * @param layout The new layout. 
     */
    void addUserLayout( const QString &name, PlaylistLayout layout );
    
    /**
     * Delete a layout. Checks if the layout is editable (is not one of the defaults) and deletes it if possible
     * @param layout The name of the layout to delete.
     */
    void deleteLayout( const QString &layout );
    
    /**
     * Check if a layout can be deleted (is not a default layout).
     * @param layout The name of the layout.
     * @return Can this layout be delete.
     */
    bool isDeleteable( const QString &layout ) const;

    /**
     * Move the named layout up one place if possible
     * @param layout The name of he layout to move
     * @return The new row of the layout
     */
    int moveUp( const QString &layout );

    /**
     * Move the named layout down one place if possible
     * @param layout The name of he layout to move
     * @return The new row of the layout
     */
    int moveDown( const QString &layout );

signals:
    
    /**
     * Signal emitted when the active layout changes.
     */
    void activeLayoutChanged();
    
    /**
     * Signal emitted when the list of layouts has changed. Either because a layout has been added or
     * removed or because the order of the layouts has changed.
     */
    void layoutListChanged();

protected:
    /**
     * Private constructor (Singleton Pattern).
     */
    LayoutManager();

    /**
     * Load the default layouts shipped with Amarok.
     */
    void loadDefaultLayouts();
    
    /**
     * Load user generated layouts.
     */
    void loadUserLayouts();
    
    /**
     * Order the layouts according to what is stored in the config. If any layouts are present that are not mentioned in the config
     * these are added to the bottom of the list, and of any layouts are mentioned in the config that is not found, these are ignored.
     */
    void orderLayouts();

    /**
     * Save the current ordering of the layouts to config.
     */
    void storeLayoutOrdering();

    /**
     * Load all layouts from an XML file.
     * @param fileName The file to load the layouts from.
     * @param user Are these to be treated as user generated layouts or Amarok default layouts.
     */
    void loadLayouts( const QString &fileName, bool user );

    /**
     * Create a DOM element corrosponding to a LayoutItemConfig. Used when storing a layout to a file.
     * @param doc The QDomDocument that is the parent of the current tree.
     * @param name The name of the layoutItem
     * @param item The layout item.
     * @return A DOM element containig the XML encoded layout item.
     */
    QDomElement createItemElement( QDomDocument doc, const QString &name, const LayoutItemConfig &item ) const;
    
    /**
     * Create a LayoutItemConfig based on a QDomElement. Use when reading a layout from an XML file.
     * @param elem The DOM element containing the encoded layout item.
     * @return The layout item generated.
     */
    LayoutItemConfig parseItemConfig( const QDomElement &elem ) const;

    static LayoutManager *s_instance;

    QMap<QString, PlaylistLayout> m_layouts;
    QStringList                   m_layoutNames;  //used to make a custom ordering of items 
    QString                       m_activeLayout;
    PlaylistLayout                m_previewLayout;
};

}

#endif
