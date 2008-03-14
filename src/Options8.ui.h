// (c) 2006 Seb Ruiz <me@sebruiz.net>

/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

void Options8::updateServices( const QString &s )
{
    bool empty = s.isEmpty();
    groupBox2->setEnabled( !empty );
    kcfg_SubmitPlayedSongs->setEnabled( !empty );
    kcfg_RetrieveSimilarArtists->setEnabled( !empty );
}

