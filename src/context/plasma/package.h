/******************************************************************************
*   Copyright 2007 by Aaron Seigo <aseigo@kde.org>                        *
*   Copyright 2007 by Riccardo Iaconelli <riccardo@kde.org>               *
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

#ifndef PLASMA_PACKAGE_H
#define PLASMA_PACKAGE_H

#include <QtCore/QStringList>

#include <plasma/plasma_export.h>
#include <plasma/packagestructure.h>

namespace Plasma
{

/**
 * @class Package plasma/package.h <Plasma/Package>
 *
 * @short object representing an installed Plasmagik package
 **/

class PackageMetadata;
class PackagePrivate;

class PLASMA_EXPORT Package
{
    public:
        /**
         * Default constructor
         *
         * @arg packageRoot path to the package installation root
         * @arg package the name of the package
         * @arg structure the package structure describing this package
         **/
        Package(const QString &packageRoot, const QString &package,
                PackageStructure::Ptr structure);

        /**
          * Construct a Package object.
          *
          * @arg packagePath full path to the package directory
          * @arg structure the package structure describing this package
          */
        Package(const QString &packagePath, PackageStructure::Ptr structure);

        //TODO for 4.1: be able to load an uninstalled/uncompressed file.

        ~Package();

        /**
         * @return true if all the required components as defined in
         * the PackageStructure exist
         **/
        bool isValid() const;

        /**
         * Get the path to a given file.
         *
         * @arg fileType the type of file to look for, as defined in the
         *               package structure
         * @arg filename the name of the file
         * @return path to the file on disk. QString() if not found.
         **/
        QString filePath(const char *fileType, const QString &filename) const;

        /**
         * Get the path to a given file.
         *
         * @arg fileType the type of file to look for, as defined in the
         *               package structure. The type must refer to a file
         *               in the package structure and not a directory.
         * @return path to the file on disk. QString() if not found
         **/
        QString filePath(const char *fileType) const;

        /**
         * Get the list of files of a given type.
         *
         * @arg fileType the type of file to look for, as defined in the
         *               package structure.
         * @return list of files by name, suitable for passing to filePath
         **/
        QStringList entryList(const char *fileType) const;

        /**
          * @return the package metadata object.
          */
        PackageMetadata metadata() const;

        /**
         * @return the path to the root of this particular package
         */
        const QString path() const;

        /**
         * @return the PackageStructure use in this Package
         */
        const PackageStructure::Ptr structure() const;

        /**
         * Returns a list of all installed packages
         *
         * @param packageRoot path to the directory where Plasmagik packages
         *                    have been installed to
         * @return a list of installed Plasmagik packages
         **/
        static QStringList listInstalled(const QString &packageRoot);

        /**
         * Installs a package.
         *
         * @param package path to the Plasmagik package
         * @param packageRoot path to the directory where the package should be
         *                    installed to
         * @return true on successful installation, false otherwise
         **/
        static bool installPackage(const QString &package,
                                   const QString &packageRoot,
                                   const QString &servicePrefix);

        /**
         * Uninstalls a package.
         *
         * @param package path to the Plasmagik package
         * @param packageRoot path to the directory where the package should be
         *                    installed to
         * @return true on successful uninstallation, false otherwise
         **/
        static bool uninstallPackage(const QString &package,
                                     const QString &packageRoot,
                                     const QString &servicePrefix);

        /**
         * Registers a package described by the given desktop file
         *
         * @arg the full path to the desktop file (must be KPluginInfo compatible)
         * @return true on success, false on failure
         */
        static bool registerPackage(const PackageMetadata &data, const QString &iconPath);

        /**
         * Creates a package based on the metadata from the files contained
         * in the source directory
         *
         * @arg metadata description of the package to create
         * @arg source path to local directory containing the individual
         *             files to be added to the package
         * @arg destination path to the package that should be created
         * @arg icon path to the package icon
         **/
        static bool createPackage(const PackageMetadata &metadata,
                                  const QString &source,
                                  const QString &destination,
                                  const QString &icon = QString());

    private:
        Q_DISABLE_COPY(Package)
        PackagePrivate * const d;
};

} // Namespace

#endif

