/****************************************************************************************
 * Copyright (c) 2009 Jakob Kummerow <jakob.kummerow@gmail.com>                         *
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

#include "signer.h"

#include "../src/dialogs/ScriptUpdater.h"

#include <QFile>
#include <QTimer>
#include <iostream>

#include <pwd.h>

signer::signer()
{
}

signer::~signer()
{}

bool signer::setParams(int argc, char** argv) 
{
	if (argc > 1) {

		// first code block: generate key pair and save to local files
		if ( strcmp( argv[1], "keygen" ) == 0 ) {
			QTimer::singleShot( 0, this, SLOT( keygen() ) );
			return 1;

		// second code block: sign a given file
		} else if ( strcmp( argv[1], "sign" ) == 0 ) {
			m_privkeyFilename = argv[2];
			QTimer::singleShot( 0, this, SLOT( signFile() ) );

			return 1;
		// third code block: check a given file
		} else if ( strcmp( argv[1], "check" ) == 0 ) {
			m_pubkeyFilename = argv[2];
			QTimer::singleShot( 0, this, SLOT( checkSignature() ) );
			return 1;
		}
	} 
	std::cout << "Usage: \n"
	"\"" << argv[0] << " keygen\" to create a new key pair and store it in the \n"
	"\tcurrent directory\n"
	
	"\"" << argv[0] << " sign <privateKey>\" to create a signature for both \n"
	"\t'" << archiveFilename.toLatin1().data() << "' and '" << versionFilename.toLatin1().data() << "' and store it as '" << signatureFilename.toLatin1().data() << "', using \n"
	"\tthe private key file <privateKey>.\n"
	
	"\"" << argv[0] << " check <publicKey>\" to check if the signature found in \n"
	"\tthe file '" << signatureFilename.toLatin1().data() << "' matches both '" << archiveFilename.toLatin1().data() << "' and '" << versionFilename.toLatin1().data() << "', \n"
	"\tusing the public key file <publicKey>\n";
	return false;
}

void 
signer::keygen()
{
	QCA::Initializer init;
	// only start this "loop" so that we can jump to the end of it using "continue"
	for ( int temp = 0; temp < 1; temp++ ) {
		if( !QCA::isSupported( "pkey" ) || !QCA::PKey::supportedIOTypes().contains( QCA::PKey::RSA ) )
		{
			std::cout << "RSA not supported on this system!\n";
			continue;
		}
		QCA::PrivateKey seckey = QCA::KeyGenerator().createRSA( 2048 );
        if( seckey.isNull() ) {
            std::cout << "Failed to make private RSA key!\n";
            continue;
        }
		QCA::PublicKey pubkey = seckey.toPublicKey();
		QCA::SecureArray passPhrase( getpass( "Please enter a passphrase for the new private key: " ) );
		QCA::SecureArray passPhraseConfirm( getpass( "Please confirm the passphrase: " ) );
		if ( passPhrase != passPhraseConfirm ) 
		{
			std::cout << "Passphrases do not match, aborting.\n";
			continue;
		}
		seckey.toPEMFile( "privkey.pem", passPhrase );
		pubkey.toPEMFile( "pubkey.pem" );
		std::cout << "Keys generated and saved to file; have fun!\n";
	}
	this->thread()->quit();
}

void 
signer::signFile()
{
	std::cout << "signing file " << archiveFilename.toLatin1().data() << "...\n";
	QCA::Initializer init;
	// only start this "loop" so that we can jump to the end of it using "continue"
	for ( int temp = 0; temp < 1; temp++ ) {
		QCA::SecureArray passPhrase( getpass("Please enter the passphrase for the private key: ") );
		QCA::ConvertResult conversionResult;
        QCA::PrivateKey seckey = QCA::PrivateKey::fromPEMFile( m_privkeyFilename, passPhrase, &conversionResult );
        if (! ( QCA::ConvertGood == conversionResult ) ) {
            std::cout << "Failed to read private key!\n";
			continue;
        }
		QFile file( archiveFilename );
		if ( !file.open( QIODevice::ReadOnly ) ) {
			std::cout << "Failed to open archive file for reading!\n";
			continue;
		}
		QCA::Hash hash( "sha1" );
		hash.update( &file );
		file.close();
		QFile versionFile( versionFilename );
		if ( !versionFile.open( QIODevice::ReadOnly ) ) {
			std::cout << "faild to open version file for reading!\n";
			continue;
		}
		QCA::Hash versionHash( "sha1" );
		versionHash.update( &versionFile );
		versionFile.close();
		seckey.startSign( QCA::EMSA3_SHA1 );
		seckey.update( hash.final() );
		seckey.update( versionHash.final() );
		QByteArray signature = seckey.signature();
		QFile sigFile( signatureFilename );
		if ( !sigFile.open(QIODevice::WriteOnly) ) {
			std::cout << "Failed to open signature file for writing!\n";
			continue;
		}
		sigFile.write( signature.toBase64() );
		sigFile.close();
		std::cout << "Signature written to " << sigFile.fileName().toLatin1().data() << "\n";
	}
	this->thread()->quit();
}

void 
signer::checkSignature()
{
	std::cout << "checking file " << archiveFilename.toLatin1().data() << "...\n";
	QCA::Initializer init;
	// only start this "loop" so that we can jump to the end of it using "continue"
	for ( int temp = 0; temp < 1; temp++ ) {
		QFile pubkeyfile( m_pubkeyFilename );
		if ( !pubkeyfile.open( QIODevice::ReadOnly ) ) 
		{
			std::cout << "Failed to open public key file!\n";
			continue;
		}
		QString pubkeystring = pubkeyfile.readAll();
		pubkeyfile.close();
		QCA::ConvertResult conversionResult;
		QCA::PublicKey pubkey = QCA::PublicKey::fromPEM( pubkeystring, &conversionResult );
		if ( !( QCA::ConvertGood == conversionResult ) )
		{
			std::cout << "Failed to read public key!\n";
			continue;
		}
		QFile file( archiveFilename );
		if ( !file.open( QIODevice::ReadOnly ) ) {
			std::cout << "Failed to open archive file for reading!\n";
			continue;
		}
		QCA::Hash hash( "sha1" );
		hash.update( &file );
		file.close();
		QFile versionFile( versionFilename );
		if ( !versionFile.open( QIODevice::ReadOnly ) ) {
			std::cout << "Faild to open version file for reading!\n";
			continue;
		}
		QCA::Hash versionHash( "sha1" );
		versionHash.update( &versionFile );
		versionFile.close();
		QFile sigFile( signatureFilename );
		if ( !sigFile.open( QIODevice::ReadOnly ) )
		{
			std::cout << "Failed to open signature file for reading!\n";
			continue;
		}
		QByteArray signature = QByteArray::fromBase64( sigFile.readAll() );
		pubkey.startVerify( QCA::EMSA3_SHA1 );
		pubkey.update( hash.final() );
		pubkey.update( versionHash.final() );
		if ( !pubkey.validSignature( signature ) ) 
		{
			std::cout << "Invalid signature!\n";
			continue;
		}
		std::cout << "Signature verified :-)\n";
	}
	this->thread()->quit();
}
		
#include "signer.moc"
