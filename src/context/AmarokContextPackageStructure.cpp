/*
 * Copyright (C) 2017  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "AmarokContextPackageStructure.h"

#include <KLocalizedString>

void AmarokContextPackageStructure::initPackage(KPackage::Package* package)
{

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    package->addFileDefinition("mainscript", QStringLiteral("ui/main.qml"), i18n("Main Script File"));
    package->addFileDefinition("icon", QStringLiteral("images/icon.png"), i18n("Applet Icon File"));
    package->addFileDefinition("icon", QStringLiteral("images/icon.svg"), i18n("Applet Icon File"));
    package->setDefaultPackageRoot(QStringLiteral("kpackage/amarok"));
    package->addDirectoryDefinition("images", QStringLiteral("images"), i18n("Images"));
#else
    package->addFileDefinition("mainscript", QStringLiteral("ui/main.qml"));
    package->addFileDefinition("icon", QStringLiteral("images/icon.png"));
    package->addFileDefinition("icon", QStringLiteral("images/icon.svg"));
    package->setDefaultPackageRoot(QStringLiteral("kpackage/amarok"));
    package->addDirectoryDefinition("images", QStringLiteral("images"));
#endif
    auto mimetypes = QStringList() << QStringLiteral("image/svg+xml");
    package->setMimeTypes("images", mimetypes);
}
