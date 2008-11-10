/*
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>
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

#ifndef PLASMA_CONFIGLOADER_H
#define PLASMA_CONFIGLOADER_H

#include <KDE/KConfigGroup>
#include <KDE/KConfigSkeleton>
#include <KDE/KSharedConfig>

#include <plasma/plasma_export.h>

/**
 * @class ConfigLoader plasma/configloader.h <Plasma/ConfigLoader>
 *
 * @short A KConfigSkeleton that populates itself based on KConfigXT XML
 *
 * This class allows one to ship an XML file and reconstitute it into a
 * KConfigSkeleton object at runtime. Common usage might look like this:
 *
 * \code
 * QFile file(xmlFilePath);
 * Plasma::ConfigLoader appletConfig(configFilePath, &file);
 * \endcode
 *
 * Alternatively, any QIODevice may be used in place of QFile in the
 * example above.
 *
 * Currently the following data types are supported:
 *
 * @li bools
 * @li colors
 * @li datetimes
 * @li enumerations
 * @li fonts
 * @li ints
 * @li passwords
 * @li paths
 * @li strings
 * @li stringlists
 * @li uints
 * @li urls
 * @li doubles
 * @li int lists
 * @li longlongs
 * @li path lists
 * @li points
 * @li rects
 * @li sizes
 * @li ulonglongs
 * @li url lists
 **/

namespace Plasma
{

class ConfigLoaderPrivate;

class PLASMA_EXPORT ConfigLoader : public KConfigSkeleton
{
public:
    /**
     * Creates a KConfigSkeleton populated using the definition found in
     * the XML data passed in.
     *
     * @param configFile path to the configuration file to use
     * @param xml the xml data; must be valid KConfigXT data
     * @param parent optional QObject parent
     **/
    ConfigLoader(const QString &configFile, QIODevice *xml, QObject *parent = 0);

    /**
     * Creates a KConfigSkeleton populated using the definition found in
     * the XML data passed in.
     *
     * @param config the configuration object to use
     * @param xml the xml data; must be valid KConfigXT data
     * @param parent optional QObject parent
     **/
    ConfigLoader(KSharedConfigPtr config, QIODevice *xml, QObject *parent = 0);

    /**
     * Creates a KConfigSkeleton populated using the definition found in
     * the XML data passed in.
     *
     * @param config the group to use as the root for configuration items
     * @param xml the xml data; must be valid KConfigXT data
     * @param parent optional QObject parent
     **/
    ConfigLoader(const KConfigGroup *config, QIODevice *xml, QObject *parent = 0);
    ~ConfigLoader();

    /**
     * Finds the item for the given group and key.
     *
     * @arg group the group in the config file to look in
     * @arg key the configuration key to find
     * @return the associated KConfigSkeletonItem, or 0 if none
     */
    KConfigSkeletonItem *findItem(const QString &group, const QString &key);

    /**
     * Check to see if a group exists
     *
     * @param group the name of the group to check for
     * @return true if the group exists, or false if it does not
     */
    bool hasGroup(const QString &group) const;

    /**
     * @return the list of groups defined by the XML
     */
    QStringList groupList() const;

private:
    ConfigLoaderPrivate * const d;
};

} // Plasma namespace

#endif //multiple inclusion guard
