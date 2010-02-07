/****************************************************************************************
 * Copyright (c) 2006 Mike Diehl <madpenguin8@yahoo.com>                                *
 * Copyright (c) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#ifndef AMAROK_ORGANIZECOLLECTIONDIALOG_H
#define AMAROK_ORGANIZECOLLECTIONDIALOG_H

#include "amarok_export.h"
#include "meta/Meta.h"
#include "FilenameLayoutDialog.h"
#include "widgets/TokenPool.h"

#include <KDialog>
#include <KVBox>

#include <QtGui/QWidget>

namespace Ui
{
    class OrganizeCollectionDialogBase;
}

class AMAROK_EXPORT OrganizeCollectionDialog : public KDialog
{
    Q_OBJECT

    public:

        explicit OrganizeCollectionDialog( const Meta::TrackList &tracks, const QStringList &folders, QWidget *parent=0, const char *name=0,
                                           bool modal=true, const QString &caption=QString(),
                                           QFlags<KDialog::ButtonCode> buttonMask=Ok|Cancel );

        ~OrganizeCollectionDialog();

        QMap<Meta::TrackPtr, QString> getDestinations();
        bool overwriteDestinations() const;

    signals:
        void updatePreview(QString);

    public slots:
        void slotUpdatePreview();
        void slotDialogAccepted();

    private:
        QString buildDestination( const QString &format, const Meta::TrackPtr &track ) const;
        QString cleanPath( const QString &component ) const;
        QString buildFormatTip() const;
        QString buildFormatString() const;
        QString commonPrefix( const QStringList &list ) const;
        void toggleDetails();
        void preview( const QString &format );
        void update( int dummy );
        void update( const QString & dummy );
        void init();

        Ui::OrganizeCollectionDialogBase *ui;
        FilenameLayoutDialog *m_filenameLayoutDialog;
        Meta::TrackPtr m_previewTrack;
        bool m_detailed;
        Meta::TrackList m_allTracks;

    private slots:
        void slotEnableOk( const QString & currentCollectionRoot );
};

#endif  //AMAROK_ORGANIZECOLLECTIONDIALOG_H
