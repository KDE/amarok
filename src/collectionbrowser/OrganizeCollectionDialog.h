// Mike Diehl (C) 2006 madpenguin8@yahoo.com
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_ORGANIZECOLLECTIONDIALOG_H
#define AMAROK_ORGANIZECOLLECTIONDIALOG_H

#include "meta/Meta.h"

#include <KDialog>

#include <QtGui/QWidget>

namespace Ui
{
    class OrganizeCollectionDialogBase;
}

class OrganizeCollectionDialog : public KDialog
{
    Q_OBJECT
    public:
    explicit OrganizeCollectionDialog(QueryMaker *qm, QWidget *parent=0, const char *name=0, bool modal=true,
            const QString &caption=QString(),
            QFlags<KDialog::ButtonCode> buttonMask=Ok|Cancel|Details
            );

    ~OrganizeCollectionDialog();
    signals:
        void detailsClicked();
        void updatePreview(QString);
    public slots:
        void slotDetails(); 
    private:

    QString buildDestination( const QString &format, const Meta::TrackPtr track ) const;
    QString cleanPath( const QString &component ) const;
    QString buildFormatTip() const;
    QString buildFormatString() const;
    void setPreviewTrack( const Meta::TrackPtr track );
    void preview( const QString &format );
    void update( int dummy );
    void update( const QString & dummy );
    void init();

    Ui::OrganizeCollectionDialogBase *ui;
    Meta::TrackPtr m_previewTrack;
    bool detailed;
};

#endif
