/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "AlbumActionsCapability.h"

#include "core/meta/Meta.h"
#include "covermanager/CoverFetchingActions.h"

#include <QIcon>
#include <KLocalizedString>

class CompilationAction : public QAction
{
    Q_OBJECT

    public:
        CompilationAction( QObject* parent, Meta::AlbumPtr album )
                : QAction( parent )
                , m_album( album )
            {
                connect( this, SIGNAL(triggered(bool)), SLOT(slotTriggered()) );
                if( m_album->isCompilation() )
                {
                    setIcon( QIcon::fromTheme( "filename-artist-amarok" ) );
                    setText( i18n( "Do not show under Various Artists" ) );
                }
                else
                {
                    setIcon( QIcon::fromTheme( "similarartists-amarok" ) );
                    setText( i18n( "Show under Various Artists" ) );
                }
                setEnabled( m_album->canUpdateCompilation() );
            }

    private slots:
        void slotTriggered()
        {
            if( !m_album->canUpdateCompilation() )
                return;
            m_album->setCompilation( !m_album->isCompilation() );
        }

    private:
        Meta::AlbumPtr m_album;
};

using namespace Capabilities;

AlbumActionsCapability::AlbumActionsCapability( Meta::AlbumPtr album, QList<QAction *> actions )
    : ActionsCapability()
{
    m_actions.append( new DisplayCoverAction( 0, album ) );
    m_actions.append( new FetchCoverAction( 0, album ) );
    m_actions.append( new SetCustomCoverAction( 0, album ) );
    m_actions.append( new UnsetCoverAction( 0, album ) );

    QAction *separator = new QAction( 0 );
    separator->setSeparator( true );
    m_actions.append( separator );
    m_actions.append( new CompilationAction( 0, album ) );

    if( actions.isEmpty() )
        return;
    separator = new QAction( 0 );
    separator->setSeparator( true );
    m_actions.append( separator );
    m_actions.append( actions );
}

AlbumActionsCapability::~AlbumActionsCapability()
{
    // nothing to do
}

#include "AlbumActionsCapability.moc"
