/******************************************************************************
*   Copyright 2007 by Riccardo Iaconelli  <riccardo@kde.org>                  *
*                                                                             *
*   This library is free software; you can redistribute it and/or             *
*   modify it under the terms of the GNU Library General Public               *
*   License as published by the Free Software Foundation; either              *
*   version 2 of the License, or (at your option) any later version.          *
*                                                                             *
*   This library is distributed in the hope that it will be useful,           *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU          *
*   Library General Public License for more details.                          *
*                                                                             *
*   You should have received a copy of the GNU Library General Public License *
*   along with this library; see the file COPYING.LIB.  If not, write to      *
*   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
*   Boston, MA 02110-1301, USA.                                               *
*******************************************************************************/

#ifndef PLASMA_PACKAGEMETADATA_H
#define PLASMA_PACKAGEMETADATA_H

#include <QtCore/QString>

#include <plasma/plasma_export.h>

namespace Plasma
{

class PackageMetadataPrivate;

/**
 * @class PackageMetadata plasma/packagemetadata.h <Plasma/PackageMetadata>
 *
 * @short Provides metadata for a Package.
 **/
class PLASMA_EXPORT PackageMetadata
{
public:
    /**
     * Default constructor
     **/
    PackageMetadata();

    /**
     * Constructs a metadata object using the values in the file at path
     *
     * @param path path to a metadata.desktop file
     **/
    PackageMetadata(const QString &path);
    ~PackageMetadata();

    bool isValid() const;

    /**
     * Writes out the metadata to filename, which should be a .desktop
     * file. It writes out the information in a format that is compatible
     * with KPluginInfo
     * @see KPluginInfo
     *
     * @arg filename path to the file to write to
     **/
    void write(const QString &filename) const;

    /**
     * Reads in metadata from a file, which should be a .desktop
     * file. It writes out the information in a format that is compatible
     * with KPluginInfo
     * @see KPluginInfo
     *
     * @arg filename path to the file to write to
     **/
    void read(const QString &filename);

    QString name() const;
    QString description() const;
    QString serviceType() const;
    QString author() const;
    QString email() const;
    QString version() const;
    QString website() const;
    QString license() const;
    QString application() const;
    QString category() const;
    QString requiredVersion() const;
    QString pluginName() const;
    QString implementationApi() const;

    QString type() const;

    /**
     * Set the name of the package used to displayed
     * a short describing name.
     */
    void setName(const QString &);

    /**
     * Set the description used to provide some general
     * information what the package is about.
     */
    void setDescription(const QString &);

    /**
     * Set the service-type which defines the X-KDE-ServiceTypes
     * type within the desktop file. If not defined this
     * defaults to "Plasma/Applet,Plasma/Containment" in the
     * desktop file.
     */
    void setServiceType(const QString &);

    /**
     * Set the name of the author of the package.
     */
    void setAuthor(const QString &);

    /**
     * Set the E-Mail address of the author or of the project
     * that provided the package.
     */
    void setEmail(const QString &);

    /**
     * Set the version of the package.
     */
    void setVersion(const QString &);

    /**
     * Set the website URL where the package is hosted or
     * where additional details about the project are available.
     */
    void setWebsite(const QString &);

    /**
     * Set the license the package is distributed under.
     */
    void setLicense(const QString &);

    /**
     * Set the name of the application this package may
     * belongs to. This is used only for display purposes
     * so far.
     */
    void setApplication(const QString &);

    /**
     * Sets the category this package belongs in
     */
    void setCategory(const QString &);

    /**
     * Set the required version. See also the setVersion()
     * method.
     */
    void setRequiredVersion(const QString &);

    /**
     * Set the type of the package. If not defined this
     * defaults to "Service" in the desktop file.
     */
    void setType(const QString &type);

    /**
     * Set the plugin name of the package.
     *
     * The plugin name is used to locate the package;
     * @code
     * QString serviceName("plasma-applet-" + data.pluginName());
     * QString service = KStandardDirs::locateLocal("services", serviceName + ".desktop");
     * @endcode
     */
    void setPluginName(const QString &name);

    /**
     * Set the implementation API this package uses.
     */
    void setImplementationApi(const QString &api);

private:
    PackageMetadataPrivate * const d;
};

}
#endif
