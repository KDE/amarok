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

#include "RhythmboxConfigWidget.h"
#include "RhythmboxProvider.h"

#include <KLocalizedString>

AMAROK_EXPORT_SIMPLE_IMPORTER_PLUGIN( RhythmboxImporterFactory,
                                      "amarok_importer-rhythmbox.json",
                                      QStringLiteral("RhythmboxImporter"),
                                      i18n( "Rhythmbox" ),
                                      i18n( "Rhythmbox Statistics Importer" ),
                                      QIcon::fromTheme( QStringLiteral("view-importers-rhythmbox-amarok") ),
                                      StatSyncing::RhythmboxConfigWidget,
                                      StatSyncing::RhythmboxProvider )

#include <RhythmboxManager.moc>
