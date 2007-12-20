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

class OrganizeCollectionDialogBase : public KDialog
{
    Q_OBJECT
    public:
    explicit OrganizeCollectionDialogBase( QWidget *parent=0, const char *name=0, bool modal=true,
            const QString &caption=QString(),
            QFlags<KDialog::ButtonCode> buttonMask=Ok|Apply|Cancel )
        : KDialog( parent )
    {
        Q_UNUSED( name )
        setCaption( caption );
        setModal( modal );
        setButtons( buttonMask );
        showButtonSeparator( true );
    }

    signals:
        void detailsClicked();
    public slots:
        void slotDetails() { KDialog::slotButtonClicked( Details ); emit detailsClicked(); adjustSize(); }
};

#endif
