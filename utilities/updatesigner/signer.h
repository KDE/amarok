#ifndef signer_H
#define signer_H

#include <QtCore/QObject>
#include <QtCrypto>

class signer : public QObject
{
	Q_OBJECT
	public:
		signer();
		virtual ~signer();
		bool setParams(int argc, char** argv);
	
	public slots:
		void keygen();
		void signFile();
		void checkSignature();
		
	private:
		QString m_fileName;
		QString m_pubkey;
};

#endif // signer_H
