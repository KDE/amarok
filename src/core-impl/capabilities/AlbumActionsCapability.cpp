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
        CompilationAction( QObject* parent, const Meta::AlbumPtr &album )
                : QAction( parent )
                , m_album( album )
            {
                connect( this, &CompilationAction::triggered, this, &CompilationAction::slotTriggered );
                if( m_album->isCompilation() )
                {
                    setIcon( QIcon::fromTheme( QStringLiteral("filename-artist-amarok") ) );
                    setText( i18n( "Do not show under Various Artists" ) );
                }
                else
                {
                    setIcon( QIcon::fromTheme( QStringLiteral("similarartists-amarok") ) );
                    setText( i18n( "Show under Various Artists" ) );
                }
                setEnabled( m_album->canUpdateCompilation() );
            }

    private Q_SLOTS:
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

AlbumActionsCapability::AlbumActionsCapability( const Meta::AlbumPtr &album, const QList<QAction *> &actions )
    : ActionsCapability()
{
    m_actions.append( new DisplayCoverAction( nullptr, album ) );
    m_actions.append( new FetchCoverAction( nullptr, album ) );
    m_actions.append( new SetCustomCoverAction( nullptr, album ) );
    m_actions.append( new UnsetCoverAction( nullptr, album ) );

    QAction *separator = new QAction( nullptr );
    separator->setSeparator( true );
    m_actions.append( separator );
    m_actions.append( new CompilationAction( nullptr, album ) );

    if( actions.isEmpty() )
        return;
    separator = new QAction( nullptr );
    separator->setSeparator( true );
    m_actions.append( separator );
    m_actions.append( actions );
}

AlbumActionsCapability::~AlbumActionsCapability()
{
    // nothing to do
}

#include "AlbumActionsCapability.moc"
