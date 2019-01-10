/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef AMAROK_SCRIPT_H
#define AMAROK_SCRIPT_H

#include <QStringList>
#include <QObject>

class QScriptEngine;

namespace AmarokScript
{
    // SCRIPTDOX: Amarok
    class AmarokScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokScript( const QString &name, QScriptEngine *engine );

            /** Shuts down Amarok completely. */
            Q_INVOKABLE void quitAmarok();

            /** 
             * Print debug output to the shell. Only printed if amarok is started with --debug.
             * @param text The text to print.
             */
            Q_INVOKABLE void debug( const QString& text ) const;

            /**
              * Show an information dialog in Amarok.
              * @param text The text to display.
              * @param type Type of the dialog. See KMessageBox docs.
              */
            Q_INVOKABLE int alert( const QString& text, const QString& type = QStringLiteral("information") ) const;

            /** Signals Amarok that this script has ended. */
            Q_INVOKABLE void end();

            /**
              * Start another Amarok script.
              * @name Name of the script to start.
              */
            Q_INVOKABLE bool runScript( const QString& name ) const;

            /**
              * Stop another Amarok script.
              * @name Name of the script to stop.
              */
            Q_INVOKABLE bool stopScript( const QString& name ) const;

            /**
             * A list of names of the currently running scripts.
             * Does not list scripts running in the scriptconsole!
             */
            Q_INVOKABLE QStringList listRunningScripts() const;

        Q_SIGNALS:
            /**
             * Emitted when this script is uninstalled.
             */
            void uninstalled();

            // TODO: actually Q_EMIT this signal
            void configured();

        private:
            QString m_name;
    };
}

#endif
