/****************************************************************************************
 * Copyright (c) 2011 Emmanuel Wagner <manu.wagner@sfr.fr>                              *
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

#ifndef COVERGRID_APPLET_H
#define COVERGRID_APPLET_H

#include "context/Applet.h"
#include "ui_CoverGridSettings.h"
#include "core/meta/forward_declarations.h"

class KConfigDialog;
class QGraphicsProxyWidget;
class QGraphicsGridLayout;

namespace Plasma
{
    class ScrollWidget;
}

class CoverGridApplet : public Context::Applet
{
    Q_OBJECT

    public:
        CoverGridApplet( QObject* parent, const QVariantList& args );
        ~CoverGridApplet();

        void init();
        bool hasHeightForWidth() const;

    public Q_SLOTS:
        void slotAlbumQueryResult( Meta::AlbumList albums);
        void saveSettings();

    protected :
        void createConfigurationInterface(KConfigDialog *parent);

    private:
        void prepareLayout();

        QGraphicsProxyWidget * m_proxywidget;
        Plasma::ScrollWidget    *m_scroll;
        QGraphicsGridLayout * m_layout;
        Meta::AlbumList m_album_list;
        Ui::CoverGridSettings ui_Settings;
        int m_coversize;
};

AMAROK_EXPORT_APPLET( covergrid, CoverGridApplet )

#endif /* COVERGRID_APPLET_H */
