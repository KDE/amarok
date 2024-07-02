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

#include "RhythmboxConfigWidget.h"

#include <KLocalizedString>
#include <KUrlRequester>

#include <QDir>

using namespace StatSyncing;

RhythmboxConfigWidget::RhythmboxConfigWidget( const QVariantMap &config, QWidget *parent,
                                              Qt::WindowFlags f )
    : SimpleImporterConfigWidget( QStringLiteral("Rhythmbox"), config, parent, f )
{
    const QString defaultPath = QDir::toNativeSeparators(
                QDir::homePath() + QStringLiteral("/.local/share/rhythmbox/rhythmdb.xml") );

    KUrlRequester *dbField = new KUrlRequester( QUrl::fromLocalFile(defaultPath) );
    dbField->setFilter( QStringLiteral("rhythmdb.xml") );
    addField( QStringLiteral("dbPath"), i18n( "Database location" ), dbField, QStringLiteral("text") );
}

RhythmboxConfigWidget::~RhythmboxConfigWidget()
{
}
