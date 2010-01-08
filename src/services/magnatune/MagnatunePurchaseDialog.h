/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#ifndef MAGNATUNEPURCHASEDIALOG_H
#define MAGNATUNEPURCHASEDIALOG_H

#include "ui_MagnatunePurchaseDialogBase.h"
#include "MagnatuneMeta.h"

class MagnatunePurchaseDialog : public QDialog, public Ui::magnatunePurchaseDialogBase, public Meta::Observer
{
    Q_OBJECT

public:
    /**
     * Overridden constructor.
     * @param parent Pointer to the parent QWidget.
     * @param name Name of this widget.
     * @param modal Sets modal state.
     * @param fl Additional dialog flags.
     */
    explicit MagnatunePurchaseDialog( QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WFlags fl = 0 );

    /**
     * Destructor
     */
    ~MagnatunePurchaseDialog();
    /*$PUBLIC_FUNCTIONS$*/


    /**
     * Sets the album to process.
     * @param album The album to process.
     */
    void setAlbum( Meta::MagnatuneAlbum * album );

    /**
     * Loads image into the cover label.
     * @param coverFile image file to load.
     */
   // void setCover( const QString &coverFile );

    using Observer::metadataChanged;
    virtual void metadataChanged( Meta::AlbumPtr album ); //reimplemented from Observer

signals:
    /**
     * Signal emitted when all needed info has been gathered and verified to purchase using
     * a credit card.
     * @param ccNumber The credit card number.
     * @param expYear The credit card expiration year.
     * @param expMonth The credit card expiration month.
     * @param name Name of customer.
     * @param email Email of customer. Used to send verification email. Can also be used.
     * on the Magnatune.com site to re-download any previous purchases.
     * @param albumCode The album code of the album.
     * @param amount The amount to pay (in us $)
     */
    void makePurchase( const QString &ccNumber, const QString &expYear, const QString &expMonth, const QString &name, const QString &email, const QString &albumCode, int amount );

     /**
     * Signal emitted when all needed info has been gathered and verified to purchase using a gift card.
     * @param gcCode The gift card code.
     * @param name Name of customer.
     * @param email Email of customer. Used to send verification email. Can also be used.
     * on the Magnatune.com site to re-download any previous purchases.
     * @param albumCode The album code of the album.
     * @param amount The amount to pay (in us $)
         */
    void makeGiftCardPurchase( const QString &gcCode, const QString &name, const QString &email, const QString &albumCode, int amount );

    /**
     * Signal emitted if purchase operation is cancelled
     */
    void cancelled();

public slots:
    /*$PUBLIC_SLOTS$*/

private:
    /*$PRIVATE_FUNCTIONS$*/

    QString m_albumCode;

    /**
     * Helper function to verify that all entries are valid.
     * @return Returns true if all entries are valid and false otherwise.
     */
    bool verifyEntries();

protected slots:
    /*$PROTECTED_SLOTS$*/

    /**
     * Slot for recieving notification when the purchase button is clicked.
     */
    void purchase();

    /**
     * Slot for recieving notification when the cancel button is pressed.
     */
    void cancel();

    /**
     * Slot called when the dialog is closed without pressing cancel.
     */
    void reject ();
};

#endif

