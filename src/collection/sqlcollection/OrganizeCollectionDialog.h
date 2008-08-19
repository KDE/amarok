/******************************************************************************
 * Copyright (c) 2006 Mike Diehl <madpenguin8@yahoo.com>                      *
 *               2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
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

#ifndef AMAROK_ORGANIZECOLLECTIONDIALOG_H
#define AMAROK_ORGANIZECOLLECTIONDIALOG_H

#include "meta/Meta.h"
#include "dialogs/FilenameLayoutDialog.h"
#include "widgets/FilenameLayoutWidget.h"
#include "widgets/TokenListWidget.h"

#include <KDialog>
#include <KVBox>    //this can be in the .cpp if widget and vbox weren't members. maybe they don't need to be but I first have to fix the layout when resizing

#include <QtGui/QWidget>

namespace Ui
{
    class OrganizeCollectionDialogBase;
}

class OrganizeCollectionDialog : public KDialog
{
    Q_OBJECT

    public:
        explicit OrganizeCollectionDialog( QueryMaker *qm, QWidget *parent=0, const char *name=0, bool modal=true,
                                           const QString &caption=QString(),
                                           QFlags<KDialog::ButtonCode> buttonMask=Ok|Cancel|Details );

        explicit OrganizeCollectionDialog( const Meta::TrackList &tracks, QWidget *parent=0, const char *name=0,
                                           bool modal=true, const QString &caption=QString(),
                                           QFlags<KDialog::ButtonCode> buttonMask=Ok|Cancel|Details );

        ~OrganizeCollectionDialog();

        QMap<Meta::TrackPtr, QString> getDestinations();
        bool overwriteDestinations() const;

    signals:
        void updatePreview(QString);

    public slots:
        virtual void slotButtonClicked( int button );
        void slotUpdatePreview();
        void slotDialogAccepted();

    private:
        QString buildDestination( const QString &format, const Meta::TrackPtr &track ) const;
        QString cleanPath( const QString &component ) const;
        QString buildFormatTip() const;
        QString buildFormatString() const;
        void toggleDetails();
        void setPreviewTrack( const Meta::TrackPtr track );
        void preview( const QString &format );
        void update( int dummy );
        void update( const QString & dummy );
        void init();

        Ui::OrganizeCollectionDialogBase *ui;
        KVBox *vbox;
        QWidget *widget;
        FilenameLayoutDialog *filenameLayoutDialog;
        Meta::TrackPtr m_previewTrack;
        bool m_detailed;
        Meta::TrackList m_allTracks;
};

#endif  //AMAROK_ORGANIZECOLLECTIONDIALOG_H
