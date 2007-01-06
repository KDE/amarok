/*
 Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*/

#ifndef MAGNATUNEPURCHASEDIALOG_H
#define MAGNATUNEPURCHASEDIALOG_H

#include "magnatunepurchasedialogbase.h"
#include "magnatunetypes.h"

class MagnatunePurchaseDialog : public magnatunePurchaseDialogBase
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
    MagnatunePurchaseDialog( QWidget* parent = 0, const char* name = 0, bool modal = false, WFlags fl = 0 );

    /**
     * Destructor
     */
    ~MagnatunePurchaseDialog();
    /*$PUBLIC_FUNCTIONS$*/


    /**
     * Sets the album to process.
     * @param album The album to process.
     */
    void setAlbum( const MagnatuneAlbum& album );

    /**
     * Loads image into the cover label.
     * @param coverFile image file to load.
     */
    void setCover( QString coverFile );


signals:

    /**
     * Signal emitted when all needed info has been gathered and verified.
     * @param ccNumber The credit card number.
     * @param expYear The credit card expiration year.
     * @param expMonth The credit card expiration month.
     * @param name Name of customer.
     * @param email Email of customer. Used to send verification email. Can also be used.
     * on the Magnatune.com site to re-download any previous purchases.
     * @param albumCode The album code of the album.
     * @param amount The amount to pay (in us $)
     */
    void makePurchase( QString ccNumber, QString expYear, QString expMonth, QString name, QString email, QString albumCode, int amount );

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

