/****************************************************************************************
 * Copyright (c) 2009-2010 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#ifndef PLAYLISTBROWSERCATEGORY_H
#define PLAYLISTBROWSERCATEGORY_H

#include "browsers/BrowserCategory.h"

#include <QMap>
#include <QAction>

namespace Amarok {
    class PrettyTreeView;
}

class QAbstractItemDelegate;
class QTreeView;

class QAction;
class KActionMenu;
class KToolBar;

class PlaylistsInFoldersProxy;
class PlaylistsByProviderProxy;
class PlaylistBrowserFilterProxy;

namespace Playlists {
    class PlaylistProvider;
}

namespace PlaylistBrowserNS {

class PlaylistBrowserModel;

class PlaylistBrowserCategory : public BrowserCategory
{
    Q_OBJECT

public:
    static QString s_mergeViewKey;

    explicit PlaylistBrowserCategory( int playlistCategory,
                                  const QString &categoryName,
                                  const QString &configGroup,
                                  PlaylistBrowserModel *model,
                                  QWidget *parent );
    ~PlaylistBrowserCategory() override;

    QString filter() const override;
    void setFilter( const QString &filter ) override;

    /**
     * Sets the help text for the action.
     * This help text will be set for all help mechanisms:
     *  - the status-bar help text
     *  - the tooltip (for toolbar buttons)
     *  - the "WhatsThis" help text (unless one was already set)
     *
     * This is more convenient than calling all three methods with the
     * same text, and this level of abstraction can allow to change
     * the default implementation of help one day more easily.
     * Of course you can also call setStatusTip, setToolTip and setWhatsThis
     * separately for more flexibility.
     *
     */
    void setHelpText(const QString &text, QAction *qa);

protected:
    KToolBar *m_toolBar;

    /**
     * A separator in between the add-folder action and visible-source,
     * merged-view actions. Subclasses can use it to insert their specialized
     * actions at a suitable place, thus keeping the generic actions in order.
     */
    QAction *m_separator;

    QTreeView *playlistView();

private Q_SLOTS:
    void toggleView( bool mergedView );
    void slotProviderAdded( Playlists::PlaylistProvider *provider, int category );
    void slotProviderRemoved( Playlists::PlaylistProvider *provider, int category );
    void slotToggleProviderButton();

    void createNewFolder();

    void newPalette( const QPalette &palette );

private:
    void createProviderButton( const Playlists::PlaylistProvider *provider );

    KActionMenu *m_providerMenu;
    QMap<const Playlists::PlaylistProvider *, QAction *> m_providerActions;

    Amarok::PrettyTreeView *m_playlistView;

    QAction *m_addFolderAction;

    QAbstractItemDelegate *m_byProviderDelegate;
    QAbstractItemDelegate *m_defaultItemDelegate;
    PlaylistsInFoldersProxy *m_byFolderProxy;
    PlaylistsByProviderProxy *m_byProviderProxy;
    PlaylistBrowserFilterProxy *m_filterProxy;

    QString m_configGroup;
    int m_playlistCategory;
};

} //namespace PlaylistBrowserNS

//for saving it in a QVariant
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
Q_DECLARE_METATYPE( const Playlists::PlaylistProvider * )
#endif

#endif // PLAYLISTBROWSERCATEGORY_H
