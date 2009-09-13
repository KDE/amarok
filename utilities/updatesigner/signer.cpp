#include "signer.h"



#include <QFile>
#include <QTimer>
#include <iostream>

#include <pwd.h>

signer::signer()
{
	QFile pubkeyfile("pubkey.pem");
	pubkeyfile.open( QIODevice::ReadOnly );
	m_pubkey = pubkeyfile.readAll();
	pubkeyfile.close();
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
			m_fileName = argv[2];
			QTimer::singleShot( 0, this, SLOT( signFile() ) );

			return 1;
		// third code block: check a given file
		} else if ( strcmp( argv[1], "check" ) == 0 ) {
			m_fileName = argv[2];
			QTimer::singleShot( 0, this, SLOT( checkSignature() ) );
			return 1;
		}
	} 
	std::cout << "Usage: \n";
	std::cout << "\"signer keygen\"  to create a new keypair and store it in the current directory\n";
	std::cout << "\"signer sign <file>\"  to create a signature for <file> and store it as <file>.signature.\n";
	std::cout << "\tThe private key must be present in the current directory for this.\n";
	std::cout << "\"signer check <file>\"  to check the signature of <file>. Both the signature file and the\n";
	std::cout << "\tpublic key must be present in the current directory for this.\n";
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
			std::cout << "RSA not supported!\n";
			continue;
		}
		QCA::PrivateKey seckey = QCA::KeyGenerator().createRSA( 2048 );
        if( seckey.isNull() ) {
            std::cout << "Failed to make private RSA key!\n";
            continue;
        }
		QCA::PublicKey pubkey = seckey.toPublicKey();
		QCA::SecureArray passPhrase( getpass( "Please enter a passphrase for the new public key: " ) );
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
	std::cout << "signing file " << m_fileName.toLatin1().data() << "...\n";
	QCA::Initializer init;
	// only start this "loop" so that we can jump to the end of it using "continue"
	for (int temp = 0; temp < 1; temp++) {
		QCA::SecureArray passPhrase( getpass("Please enter the passphrase for the public key: ") );
		QCA::ConvertResult conversionResult;
        QCA::PrivateKey seckey = QCA::PrivateKey::fromPEMFile( "privkey.pem", passPhrase, &conversionResult );
        if (! (QCA::ConvertGood == conversionResult) ) {
            std::cout << "Failed to read private key!\n";
			continue;
        }
		QFile file(m_fileName);
		if ( !file.open(QIODevice::ReadOnly) ) {
			std::cout << "Failed to open file for reading!\n";
			continue;
		}
		QCA::Hash hash("sha1");
		hash.update(&file);
		file.close();
		QByteArray signature = seckey.signMessage( hash.final(), QCA::EMSA3_SHA1 );
		QFile sigFile( m_fileName + ".signature" );
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
	std::cout << "checking file " << m_fileName.toLatin1().data() << "...\n";
	QCA::Initializer init;
	// only start this "loop" so that we can jump to the end of it using "continue"
	for (int temp = 0; temp < 1; temp++) {
		QCA::ConvertResult conversionResult;
		QCA::PublicKey pubkey = QCA::PublicKey::fromPEM( m_pubkey, &conversionResult );
		if ( !( QCA::ConvertGood == conversionResult) )
		{
			std::cout << "Failed to read public key!\n";
			continue;
		}
		QFile file(m_fileName);
		if ( !file.open(QIODevice::ReadOnly) ) {
			std::cout << "Failed to open file for reading!\n";
			continue;
		}
		QCA::Hash hash( "sha1" );
		hash.update( &file );
		file.close();
		QFile sigFile( m_fileName + ".signature" );
		if ( !sigFile.open( QIODevice::ReadOnly ) )
		{
			std::cout << "Failed to open signature file for reading!\n";
			continue;
		}
		QByteArray signature = QByteArray::fromBase64( sigFile.readAll() );
		pubkey.startVerify( QCA::EMSA3_SHA1 );
		pubkey.update( hash.final() );
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
