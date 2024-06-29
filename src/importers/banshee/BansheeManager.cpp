/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "importers/SimpleImporterManager.h"

#include "BansheeConfigWidget.h"
#include "BansheeProvider.h"

#include <KLocalizedString>

AMAROK_EXPORT_SIMPLE_IMPORTER_PLUGIN( BansheeImporterFactory,
                                      "amarok_importer-banshee.json",
                                      QStringLiteral("BansheeImporter"),
                                      i18n( "Banshee" ),
                                      i18n( "Banshee Statistics Importer" ),
                                      QIcon::fromTheme( QStringLiteral("view-importers-banshee-amarok") ),
                                      StatSyncing::BansheeConfigWidget,
                                      StatSyncing::BansheeProvider )

#include <BansheeManager.moc>
