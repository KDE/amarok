/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "IpodDeviceHelper.h"

#include "core/support/Debug.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>

#include <KConfigGroup>
#include <KFormat>
#include <KLocalizedString>


Itdb_iTunesDB*
IpodDeviceHelper::parseItdb( const QString &mountPoint, QString &errorMsg )
{
    Itdb_iTunesDB *itdb;
    GError *error = nullptr;

    errorMsg.clear();
    itdb = itdb_parse( QFile::encodeName( mountPoint ), &error );
    if( error )
    {
        if( itdb )
            itdb_free( itdb );
        itdb = nullptr;
        errorMsg = QString::fromUtf8( error->message );
        g_error_free( error );
        error = nullptr;
    }
    if( !itdb && errorMsg.isEmpty() )
        errorMsg = i18n( "Cannot parse iTunes database due to an unreported error." );
    return itdb;
}

QString
IpodDeviceHelper::collectionName( Itdb_iTunesDB *itdb )
{
    const Itdb_IpodInfo *info = (itdb && itdb->device) ? itdb_device_get_ipod_info( itdb->device ) : nullptr;
    QString modelName = info ? QString::fromUtf8( itdb_info_get_ipod_model_name_string( info->ipod_model ) )
                             : i18nc( "iPod model that is not (yet) recognized", "Unrecognized model" );

    return i18nc( "Name of the iPod collection; %1 is iPod name, %2 is iPod model; example: My iPod: Nano (Blue)",
                  "%1: %2", IpodDeviceHelper::ipodName( itdb ), modelName );
}

QString
IpodDeviceHelper::ipodName( Itdb_iTunesDB *itdb )
{
    Itdb_Playlist *mpl = itdb ? itdb_playlist_mpl( itdb ) : nullptr;
    QString mplName = mpl ? QString::fromUtf8( mpl->name ) : QString();
    if( mplName.isEmpty() )
        mplName = i18nc( "default iPod name (when user-set name is empty)", "iPod" );

    return mplName;
}

void
IpodDeviceHelper::unlinkPlaylistsTracksFromItdb( Itdb_iTunesDB *itdb )
{
    if( !itdb )
        return;

    while( itdb->playlists )
    {
        Itdb_Playlist *ipodPlaylist = (Itdb_Playlist *) itdb->playlists->data;
        if( !ipodPlaylist || ipodPlaylist->itdb != itdb )
        {
            /* a) itdb_playlist_unlink() cannot work if ipodPlaylist is null, prevent
             *    infinite loop
             * b) if ipodPlaylist->itdb != itdb, something went horribly wrong. Prevent
             *    infinite loop even in this case
             */
            itdb->playlists = g_list_remove( itdb->playlists, ipodPlaylist );
            continue;
        }
        itdb_playlist_unlink( ipodPlaylist );
    }

    while( itdb->tracks )
    {
        Itdb_Track *ipodTrack = (Itdb_Track *) itdb->tracks->data;
        if( !ipodTrack || ipodTrack->itdb != itdb )
        {
            /* a) itdb_track_unlink() cannot work if ipodTrack is null, prevent infinite
             *    loop
             * b) if ipodTrack->itdb != itdb, something went horribly wrong. Prevent
             *    infinite loop even in this case
             */
            itdb->tracks = g_list_remove( itdb->tracks, ipodTrack );
            continue;
        }
        itdb_track_unlink( ipodTrack );
    }
}

/**
 * Return ipod info if iPod model is recognized, returns null if itdb is null or if iPod
 * is invalid or unknown.
 */
static const Itdb_IpodInfo *getIpodInfo( const Itdb_iTunesDB *itdb )
{
    if( !itdb || !itdb->device )
        return nullptr;
    const Itdb_IpodInfo *info = itdb_device_get_ipod_info( itdb->device );
    if( !info )
        return nullptr;
    if( info->ipod_model == ITDB_IPOD_MODEL_INVALID
     || info->ipod_model == ITDB_IPOD_MODEL_UNKNOWN )
    {
        return nullptr;
    }
    return info;
}

static bool
firewireGuidNeeded( const Itdb_IpodGeneration &generation )
{
    switch( generation )
    {
        // taken from libgpod itdb_device.c itdb_device_get_checksum_type()
        // not nice, but should not change, no new devices use hash58
        case ITDB_IPOD_GENERATION_CLASSIC_1:
        case ITDB_IPOD_GENERATION_CLASSIC_2:
        case ITDB_IPOD_GENERATION_CLASSIC_3:
        case ITDB_IPOD_GENERATION_NANO_3:
        case ITDB_IPOD_GENERATION_NANO_4:
            return true; // ITDB_CHECKSUM_HASH58
        default:
            break;
    }
    return false;
}

static bool
hashInfoNeeded( const Itdb_IpodGeneration &generation )
{
    switch( generation )
    {
        // taken from libgpod itdb_device.c itdb_device_get_checksum_type()
        // not nice, but should not change, current devices need libhashab
        case ITDB_IPOD_GENERATION_NANO_5:
        case ITDB_IPOD_GENERATION_TOUCH_1:
        case ITDB_IPOD_GENERATION_TOUCH_2:
        case ITDB_IPOD_GENERATION_TOUCH_3:
        case ITDB_IPOD_GENERATION_IPHONE_1:
        case ITDB_IPOD_GENERATION_IPHONE_2:
        case ITDB_IPOD_GENERATION_IPHONE_3:
            return true; // ITDB_CHECKSUM_HASH72
        default:
            break;
    }
    return false;
}

static bool
hashAbNeeded( const Itdb_IpodGeneration &generation )
{
    switch( generation )
    {
        // taken from libgpod itdb_device.c itdb_device_get_checksum_type()
        // TODO: not nice, new released devices may be added!
        case ITDB_IPOD_GENERATION_IPAD_1:
        case ITDB_IPOD_GENERATION_IPHONE_4:
        case ITDB_IPOD_GENERATION_TOUCH_4:
        case ITDB_IPOD_GENERATION_NANO_6:
            return true; // ITDB_CHECKSUM_HASHAB
        default:
            break;
    }
    return false;
}

/**
 * Returns true if file @param relFilename is found, readable and nonempty.
 * Searches in @param mountPoint /iPod_Control/Device/
 */
static bool
fileFound( const QString &mountPoint, const QString &relFilename )
{
    gchar *controlDir = itdb_get_device_dir( QFile::encodeName( mountPoint ) );
    if( !controlDir )
        return false;
    QString absFilename = QStringLiteral( "%1/%2" ).arg( QFile::decodeName( controlDir ),
                                                  relFilename );
    g_free( controlDir );

    QFileInfo fileInfo( absFilename );
    return fileInfo.isReadable() && fileInfo.size() > 0;
}

static bool
safeToWriteWithMessage( const QString &mountPoint, const Itdb_iTunesDB *itdb, QString &message )
{
    const Itdb_IpodInfo *info = getIpodInfo( itdb ); // returns null on null itdb
    if( !info )
    {
        message = i18n( "iPod model was not recognized." );
        return false;
    }

    QString gen = QString::fromUtf8( itdb_info_get_ipod_generation_string( info->ipod_generation ) );
    if( firewireGuidNeeded( info->ipod_generation ) )
    {
        // okay FireWireGUID may be in plain SysInfo, too, but it's hard to check and
        // error-prone so we just require SysInfoExtended which is machine-generated
        const QString sysInfoExtended( "SysInfoExtended" );
        bool sysInfoExtendedExists = fileFound( mountPoint, sysInfoExtended );
        message += ( sysInfoExtendedExists )
                   ? i18n( "%1 family uses %2 file to generate correct database checksum.",
                           gen, sysInfoExtended )
                   : i18n( "%1 family needs %2 file to generate correct database checksum.",
                           gen, sysInfoExtended );
        if( !sysInfoExtendedExists )
            return false;
    }
    if( hashInfoNeeded( info->ipod_generation ) )
    {
        const QString hashInfo( QStringLiteral("HashInfo") );
        bool hashInfoExists = fileFound( mountPoint, hashInfo );
        message += hashInfoExists
                   ? i18n( "%1 family uses %2 file to generate correct database checksum.",
                           gen, hashInfo )
                   : i18n( "%1 family needs %2 file to generate correct database checksum.",
                           gen, hashInfo );
        if( !hashInfoExists )
            return false;
    }
    if( hashAbNeeded( info->ipod_generation ) )
    {
        message += i18nc( "Do not translate hash-AB, libgpod, libhashab.so",
            "%1 family probably uses hash-AB to generate correct database checksum. "
            "libgpod (as of version 0.8.2) doesn't know how to compute it, but tries "
            "to dynamically load external library libhashab.so to do it.", gen
        );
        // we don't return false, user may have hash-AB support installed
    }
    return true;
}

static void
fillInModelComboBox( QComboBox *comboBox, bool someSysInfoFound )
{
    if( someSysInfoFound )
    {
        comboBox->addItem( i18n( "Autodetect (%1 file(s) present)", QStringLiteral( "SysInfo") ), QString() );
        comboBox->setEnabled( false );
        return;
    }

    const Itdb_IpodInfo *info = itdb_info_get_ipod_info_table();
    if( !info )
    {
        // this is not i18n-ed for purpose: it should never happen
        comboBox->addItem( QStringLiteral( "Failed to get iPod info table!" ), QString() );
        return;
    }

    while( info->model_number )
    {
        QString generation = QString::fromUtf8( itdb_info_get_ipod_generation_string( info->ipod_generation) );
        QString capacity = KFormat().formatByteSize( info->capacity * 1073741824.0, 0 );
        QString modelName = QString::fromUtf8( itdb_info_get_ipod_model_name_string( info->ipod_model ) );
        QString modelNumber = QString::fromUtf8( info->model_number );
        QString label = i18nc( "Examples: "
                               "%1: Nano with camera (5th Gen.); [generation]"
                               "%2: 16 GiB; [capacity]"
                               "%3: Nano (Orange); [model name]"
                               "%4: A123 [model number]",
                               "%1: %2 %3 [%4]",
                               generation, capacity, modelName, modelNumber );
        comboBox->addItem( label, modelNumber );
        info++; // list is ended by null-filled info
    }
    comboBox->setMaxVisibleItems( 16 );
}

void
IpodDeviceHelper::fillInConfigureDialog( QDialog *configureDialog,
                                         Ui::IpodConfiguration *configureDialogUi,
                                         const QString &mountPoint,
                                         Itdb_iTunesDB *itdb,
                                         const Transcoding::Configuration &transcodeConfig,
                                         const QString &errorMessage )
{
    static const QString unknown = i18nc( "Unknown iPod model, generation...", "Unknown" );
    static const QString supported = i18nc( "In a dialog: Video: Supported", "Supported" );
    static const QString notSupported = i18nc( "In a dialog: Video: Not supported", "Not supported" );
    static const QString present = i18nc( "In a dialog: Some file: Present", "Present" );
    static const QString notFound = i18nc( "In a dialog: Some file: Not found", "<b>Not found</b>" );
    static const QString notNeeded = i18nc( "In a dialog: Some file: Not needed", "Not needed" );

    // following call accepts null itdb
    configureDialogUi->nameLineEdit->setText( IpodDeviceHelper::ipodName( itdb ) );
    QString notes;
    QString warningText;
    QString safeToWriteMessage;
    bool isSafeToWrite = safeToWriteWithMessage( mountPoint, itdb, safeToWriteMessage );
    bool sysInfoExtendedExists = fileFound( mountPoint, QStringLiteral("SysInfoExtended") );
    bool sysInfoExists = fileFound( mountPoint, QStringLiteral("SysInfo") );

    if( itdb )
    {
        configureDialogUi->nameLineEdit->setEnabled( isSafeToWrite );
        configureDialogUi->transcodeComboBox->setEnabled( isSafeToWrite );
        configureDialogUi->transcodeComboBox->fillInChoices( transcodeConfig );
        configureDialogUi->modelComboLabel->setEnabled( false );
        configureDialogUi->modelComboBox->setEnabled( false );
        configureDialogUi->initializeLabel->setEnabled( false );
        configureDialogUi->initializeButton->setEnabled( false );
        if( !errorMessage.isEmpty() )
            // to inform user about successful initialization.
            warningText = QStringLiteral( "<b>%1</b>" ).arg( errorMessage );

        const Itdb_Device *device = itdb->device;
        const Itdb_IpodInfo *info = device ? itdb_device_get_ipod_info( device ) : nullptr;
        configureDialogUi->infoGroupBox->setEnabled( true );
        configureDialogUi->modelPlaceholer->setText( info ? QString::fromUtf8(
            itdb_info_get_ipod_model_name_string( info->ipod_model ) ) : unknown );
        configureDialogUi->generationPlaceholder->setText( info ? QString::fromUtf8(
            itdb_info_get_ipod_generation_string( info->ipod_generation ) ) : unknown );
        configureDialogUi->videoPlaceholder->setText( device ?
            ( itdb_device_supports_video( device ) ? supported : notSupported ) : unknown );
        configureDialogUi->albumArtworkPlaceholder->setText( device ?
            ( itdb_device_supports_artwork( device ) ? supported : notSupported ) : unknown );

        if( isSafeToWrite )
            notes += safeToWriteMessage; // may be empty, doesn't hurt
        else
        {
            Q_ASSERT( !safeToWriteMessage.isEmpty() );
            const QString link( QStringLiteral("http://gtkpod.git.sourceforge.net/git/gitweb.cgi?p=gtkpod/libgpod;a=blob_plain;f=README.overview") );
            notes += i18nc( "%1 is informational sentence giving reason",
                "<b>%1</b><br><br>"
                "As a safety measure, Amarok will <i>refuse to perform any writes</i> to "
                "iPod. (modifying iTunes database could make it look empty from the device "
                "point of view)<br>"
                "See <a href='%2'>README.overview</a> file from libgpod source repository "
                "for more information.",
                safeToWriteMessage, link
            );
        }
    }
    else
    {
        configureDialogUi->nameLineEdit->setEnabled( true ); // for initialization
        configureDialogUi->modelComboLabel->setEnabled( true );
        configureDialogUi->modelComboBox->setEnabled( true );
        if( configureDialogUi->modelComboBox->count() == 0 )
            fillInModelComboBox( configureDialogUi->modelComboBox, sysInfoExists || sysInfoExtendedExists );
        configureDialogUi->initializeLabel->setEnabled( true );
        configureDialogUi->initializeButton->setEnabled( true );
        configureDialogUi->initializeButton->setIcon( QIcon::fromTheme( QStringLiteral("task-attention") ) );
        if( !errorMessage.isEmpty() )
            warningText = i18n(
                "<b>%1</b><br><br>"
                "Above problem prevents Amarok from using your iPod. You can try to "
                "re-create critical iPod folders and files (including iTunes database) "
                "using the <b>%2</b> button below.<br><br> "
                "Initializing iPod <b>destroys iPod track and photo database</b>, however "
                "it should not delete any tracks. The tracks will become orphaned.",
                errorMessage,
                configureDialogUi->initializeButton->text().remove( QLatin1Char('&') )
            );

        configureDialogUi->infoGroupBox->setEnabled( false );
        configureDialogUi->modelPlaceholer->setText(  unknown );
        configureDialogUi->generationPlaceholder->setText(  unknown );
        configureDialogUi->videoPlaceholder->setText(  unknown );
        configureDialogUi->albumArtworkPlaceholder->setText( unknown );
    }

    if( !warningText.isEmpty() )
    {
        configureDialogUi->initializeLabel->setText( warningText );
        configureDialogUi->initializeLabel->adjustSize();
    }

    QString sysInfoExtendedString = sysInfoExtendedExists ? present : notFound;
    QString sysInfoString = sysInfoExists ? present :
                          ( sysInfoExtendedExists ? notNeeded : notFound );

    configureDialogUi->sysInfoPlaceholder->setText( sysInfoString );
    configureDialogUi->sysInfoExtendedPlaceholder->setText( sysInfoExtendedString );
    configureDialogUi->notesPlaceholder->setText( notes );
    configureDialogUi->notesPlaceholder->adjustSize();

    configureDialog->findChild<QDialogButtonBox*>()->button( QDialogButtonBox::Ok )->setEnabled( isSafeToWrite );
}

bool
IpodDeviceHelper::initializeIpod( const QString &mountPoint,
                                  const Ui::IpodConfiguration *configureDialogUi,
                                  QString &errorMessage )
{
    DEBUG_BLOCK
    bool success = true;

    int currentModelIndex = configureDialogUi->modelComboBox->currentIndex();
    QByteArray modelNumber = configureDialogUi->modelComboBox->itemData( currentModelIndex ).toString().toUtf8();
    if( !modelNumber.isEmpty() )
    {
        modelNumber.prepend( 'x' );  // ModelNumStr should start with x
        const char *modelNumberRaw = modelNumber.constData();
        Itdb_Device *device = itdb_device_new();
        // following call reads existing SysInfo
        itdb_device_set_mountpoint( device, QFile::encodeName( mountPoint ) );
        const char *field = "ModelNumStr";
        debug() << "Setting SysInfo field" << field << "to value" << modelNumberRaw;
        itdb_device_set_sysinfo( device, field, modelNumberRaw );
        GError *error = nullptr;
        success = itdb_device_write_sysinfo( device, &error );
        if( !success )
        {
            if( error )
            {
                errorMessage = i18nc( "Do not translate SysInfo",
                                      "Failed to write SysInfo: %1", error->message );
                g_error_free( error );
            }
            else
                errorMessage = i18nc( "Do not translate SysInfo",
                    "Failed to write SysInfo file due to an unreported error" );
        }
        itdb_device_free( device );
        if( !success )
            return success;
    }

    QString name = configureDialogUi->nameLineEdit->text();
    if( name.isEmpty() )
        name = ipodName( nullptr ); // return fallback name

    GError *error = nullptr;
    success = itdb_init_ipod( QFile::encodeName( mountPoint ), nullptr /* model number */,
                              name.toUtf8(), &error );
    errorMessage.clear();
    if( error )
    {
        errorMessage = QString::fromUtf8( error->message );
        g_error_free( error );
        error = nullptr;
    }
    if( !success && errorMessage.isEmpty() )
        errorMessage = i18n( "Cannot initialize iPod due to an unreported error." );
    return success;
}

void
IpodDeviceHelper::setIpodName( Itdb_iTunesDB *itdb, const QString &newName )
{
    if( !itdb )
        return;
    Itdb_Playlist *mpl = itdb_playlist_mpl( itdb );
    if( !mpl )
        return;
    g_free( mpl->name );
    mpl->name = g_strdup( newName.toUtf8() );
}

bool
IpodDeviceHelper::safeToWrite( const QString &mountPoint, const Itdb_iTunesDB *itdb )
{
    QString dummyMessage;
    return safeToWriteWithMessage( mountPoint, itdb, dummyMessage );
}
