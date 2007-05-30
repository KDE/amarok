// Mike Diehl (C) 2006 madpenguin8@yahoo.com
// See COPYING file for licensing information.

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
