/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#ifndef AMAROK_SCRIPT_H
#define AMAROK_SCRIPT_H

#include <QObject>
#include <QtScript>

namespace AmarokScript
{

    class AmarokScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokScript( QScriptEngine* ScriptEngine );
            ~AmarokScript();
            void slotConfigured();

        public slots:
            QString     Version();
            void        Quit();
            int         alert( QString text, QString type = "information" );
            bool        runScript( QString name );
            bool        stopScript( QString name );
            QStringList listRunningScripts();
        signals:
            void configured();

        private slots:

    };
}

#endif
