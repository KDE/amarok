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

#include "AmarokScript.h"

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "ScriptManager.h"

#include <KMessageBox>

namespace AmarokScript
{
    AmarokScript::AmarokScript( const QString& name )
        : QObject( kapp )
        , m_name( name )
    {}

    AmarokScript::~AmarokScript()
    {}

    void AmarokScript::quitAmarok()
    {
        kapp->quit();
    }

    void AmarokScript::debug( const QString& text ) const
    {
        ::debug() << "SCRIPT" << m_name << ": " << text;
    }

    int AmarokScript::alert( const QString& text, const QString& type ) const
    {
        //Ok = 1, Cancel = 2, Yes = 3, No = 4, Continue = 5
        if ( type == "error" )
        {
            KMessageBox::error( 0, text );
            return -1;
        }
        else if ( type == "sorry" )
        {
            KMessageBox::sorry( 0, text );
            return -1;
        }
        else if ( type == "information" )
        {
            KMessageBox::information( 0, text );
            return -1;
        }
        else if ( type == "questionYesNo" )
            return KMessageBox::questionYesNo( 0, text );
        else if ( type == "questionYesNoCancel" )
            return KMessageBox::questionYesNo( 0, text );
        else if ( type == "warningYesNo" )
            return KMessageBox::warningYesNo( 0, text );
        else if ( type == "warningContinueCancel" )
            return KMessageBox::warningContinueCancel( 0, text );
        else if ( type == "warningYesNoCancel" )
            return KMessageBox::warningYesNoCancel( 0, text );

        warning() << "alert type not found!";
        //TODO: write to error log since it's a script error
        return -1;
    }

    void AmarokScript::end()
    {
        ScriptManager::instance()->stopScript( m_name );
    }

    bool AmarokScript::runScript( const QString& name ) const
    {
        return ScriptManager::instance()->runScript( name, true );
    }

    bool AmarokScript::stopScript( const QString& name ) const
    {
        return ScriptManager::instance()->stopScript( name );
    }

    QStringList AmarokScript::listRunningScripts() const
    {
        return ScriptManager::instance()->listRunningScripts();
    }

/*
    void AmarokScript::slotConfigured()
    {
        emit configured();
    }
*/
}

#include "AmarokScript.moc"
