/***************************************************************************
 *   Copyright (C) 2005 Ian Monroe <ian@monroe.nu>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef PLAYLISTSELECTION_H
#define PLAYLISTSELECTION_H

#include <k3listview.h>

class NewDynamic;
class KDialog;
class DynamicMode;

//this is a widget used in newdynamic.ui
class PlaylistSelection : public K3ListView
{
        Q_OBJECT
    public:
        PlaylistSelection(QWidget* parent, char* name);
        virtual QSize sizeHint() const
        {
            return minimumSizeHint();
        }

    private:
        void loadChildren(Q3ListViewItem* browserParent, Q3ListViewItem* selectionParent);
};

namespace ConfigDynamic
{
    void addDynamic( NewDynamic* dialog );
    void dynamicDialog( QWidget* parent );
    void editDynamicPlaylist( QWidget* parent, DynamicMode* mode );
    void loadDynamicMode( DynamicMode* saveMe, NewDynamic* dialog );

    KDialog* basicDialog( QWidget* parent );
}

class SelectionListItem : public Q3CheckListItem
{
    public:
        SelectionListItem( Q3ListViewItem  * parent, const QString& text, Q3ListViewItem* browserEquivalent );
        SelectionListItem( Q3CheckListItem * parent, const QString& text, Q3ListViewItem* browserEquivalent );

        virtual QString name() const;

    protected:
        virtual void stateChange( bool );

    private:
        Q3ListViewItem* m_browserEquivalent;
};

#endif /*PLAYLISTSELECTION_H*/
