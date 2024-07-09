/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#ifndef SCRIPT_IMPORTER_H
#define SCRIPT_IMPORTER_H


#include <QObject>
#include <QSet>
#include <QStringList>
#include <QUrl>

namespace AmarokScript
{
    class AmarokScriptEngine;

    // SCRIPTDOX: Importer
    class ScriptImporter : public QObject
    {
        Q_OBJECT

        public:
            ScriptImporter( AmarokScriptEngine *scriptEngine, const QUrl &url );

            Q_INVOKABLE QStringList availableBindings() const;
            Q_INVOKABLE bool loadAmarokBinding( const QString &name );
            Q_INVOKABLE void loadExtension( const QString &src );
            Q_INVOKABLE bool loadQtBinding( const QString &binding );
            Q_INVOKABLE bool include( const QString &relativeFile );

        private:
            const QUrl m_scriptUrl;
            AmarokScriptEngine *m_engine;
            QSet<QString> m_importedBindings;
            bool m_qtScriptCompat = true;
    };
}

#endif
