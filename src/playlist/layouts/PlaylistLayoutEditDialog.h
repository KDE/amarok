/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#ifndef PLAYLISTLAYOUTEDITDIALOG_H
#define PLAYLISTLAYOUTEDITDIALOG_H

#include "playlist/layouts/LayoutEditWidget.h"
#include "playlist/layouts/LayoutItemConfig.h"

#include <QDialog>

#include "ui_PlaylistLayoutEditDialog.h"

namespace Playlist {

/**
 *  Dialog for adding, deleting, copying and editing playlist layouts. The order in which the layouts are
 *  shown can also be changed.
 *  @author Nikolaj Hald Nielsen <nhn@kde.org>
 */
class PlaylistLayoutEditDialog : public QDialog, private Ui::PlaylistLayoutEditDialog
{
    Q_OBJECT

    public:

        /**
         * Constructor for PlaylistLayoutEditDialog.
         * Populates the token pool, loads the available layouts from the LayoutManager in the right area and loads the configuration of the currently active layout.
         * @param parent The parent widget.
         */
        explicit PlaylistLayoutEditDialog( QWidget *parent = nullptr );

        /**
         * Destructor.
         */
        ~PlaylistLayoutEditDialog() override;

    public Q_SLOTS:

        /**
         * Set the currently selected layout.
         * Loads the configuration of the layout layoutName from the m_layoutsMap to the LayoutItemConfig area.
         * @param layoutName The name of the layout to select.
         */
        void setLayout( const QString &layoutName );

    protected Q_SLOTS:

        /**
         * Previews the current layout in the playlist without saving it.
         */
        void preview();

        /**
         * Accepts the currently changed layouts and stores them. Closes the dialog.
         */
        void accept() override;

        /**
         * Reject the changed layouts and close the dialog.
         */
        void reject() override;

        /**
         * Accepts the currently changed layouts and stores them.
         */
        void apply();

        /**
         * Creates a new PlaylistLayout with a given name and loads it in the right area to configure it.
         * The new layout is not saved in the LayoutManager but in m_layoutsMap.
         */
        void newLayout();

        /**
         * Creates a new PlaylistLayout with a given name as a copy of an existing layout and loads it in the right area to configure it.
         * The new layout is not saved in the LayoutManager but in m_layoutsMap.
         */
        void copyLayout();

        /**
         * Deletes the current layout selected in the layoutListWidget.
         */
        void deleteLayout();

        /**
         * Renames the current layout selected in the layoutListWidget.
         */
        void renameLayout();

        /**
         * Moves the currently selected layout up one place (if not already at the top). This is applied immediately.
         */
        void moveUp();

        /**
         * Moves the currently selected layout down one place (if not already at the bottom). This is applied immediately.
         */
        void moveDown();

        /**
         * Disables the delete and rename buttons if the selected layout is one of the default layouts and enables them otherwise.
         */
        void toggleEditButtons();

        /**
         * Activates/Deactivates the up and down buttons depending on whether the currently selected item can be moved up and or down.
         */
        void toggleUpDownButtons();

        /** Writes back the current UI values to the m_layoutsMap. */
        void setLayoutChanged();

    private:
        /**
         * Changes which layout tabs are enabled based on the grouping mode selected
         */
        void setEnabledTabs();

        /**
         * Populates the grouping mode combo box with options
         */
        void setupGroupByCombo();


        Playlist::LayoutEditWidget *m_partsEdit[Playlist::PlaylistLayout::NumParts];

        QMap<QString, PlaylistLayout> *m_layoutsMap;

        /** The name of the currently active layout */
        QString m_layoutName;

        QString m_firstActiveLayout;
};

}

#endif
