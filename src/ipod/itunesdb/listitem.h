/***************************************************************************
 *   Copyright (C) 2004 by Michael Schulze                                 *
 *   mike.s@genion.de                                                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef ITUNESDBLISTITEM_H
#define ITUNESDBLISTITEM_H

#include <qstring.h>
#include <qmap.h>

namespace itunesdb {

enum ItemProperty {
  MHOD_TITLE = 1,
  MHOD_PATH = 2,
  MHOD_ALBUM = 3,
  MHOD_ARTIST = 4,
  MHOD_GENRE = 5,
  MHOD_FDESC = 6,
  MHOD_EQ_SETTING = 7,
  MHOD_COMMENT = 8,
  MHODCATEGORY = 9,
  MHOD_COMPOSER = 12,
  MHOD_GROUPING = 13,
  MHOD_DESCRIPTION_TEST = 14,
  MHOD_PODCAST_ENCLOSURE_URL = 15,
  MHOD_PODCAST_RSS_URL = 16,
  MHOD_CHAPTER_DATA = 17,
  MHOD_SUBTITLE = 18,
  MHOD_SMART_PLAYLIST_INFO = 50,
  MHOD_SMART_PLAYLIST_IRULES = 51,
  MHOD_LIBRARY_PLAYLIST_INDEX = 52,
  MHOD_PLAYLIST = 100
};

enum {
  ITEMTYPE_NONE = 0,
  ITEMTYPE_TRACK = 1,
  ITEMTYPE_PLAYLISTITEM = 2,
  ITEMTYPE_PLAYLIST = 3
};    // known implementors

/**
Describes a list item in iTunesDB. Possible known subtypes at the moment are playlist, playlistitem and track

@author Michael Schulze
*/
class ListItem{
public:
    ListItem();
    ListItem( int type);
    virtual ~ListItem();

    /**
     * returns the type ofthis item
     * possible values from the known implementors are ITEMTYPE_PLAYLIST, ITEMTYPE_PLAYLISTITEM
     * and ITEMTYPE_TRACK
     */
    int getType() const;

    /**
     * sets a given itunesDB item property
     * @param data value for the property to be set
     * @param field propertyID of the property to be set
     */
    void setItemProperty(const QString& data, ItemProperty field);

    /**
     * Returns the value for the given property.
     * @param field PropertyID of the property to be returned.
     * @return the value for the given property.
     */
    const QString& getItemProperty( ItemProperty field) const;

    /**
     * Returns the number of properties.
     */
    int getNumComponents() const;

    /**
     * This method may be overridden for consistency checks after all properties have been set.
     */
    virtual void doneAddingData();

protected:
    typedef QMap<Q_UINT32,QString> PropertyMap;
    PropertyMap properties;
    int itemType;
};

}

#endif
