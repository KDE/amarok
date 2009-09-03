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

#include <KUrl>

#include <QObject>
#include <QtScript>

namespace AmarokScript
{

    class ScriptImporter : public QObject
    {
        Q_OBJECT

        public:
            ScriptImporter( QScriptEngine* ScriptEngine, KUrl url );
            ~ScriptImporter();

        public slots:
            void loadExtension( const QString& src );
            bool loadQtBinding( const QString& binding );
            bool include( const QString& relativeFile );

        private:
            const KUrl      m_scriptUrl;
            QScriptEngine*  m_scriptEngine;
            QSet<QString>   m_importedBindings;
    };
}

#endif
