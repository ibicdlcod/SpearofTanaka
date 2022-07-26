#include "client.h"

#include <QSettings>
#include <QPasswordDigestor>

#include "ecma48.h"
#include "protocol.h"

#define QT_NO_CAST_FROM_ASCII

extern QSettings *settings;

Client::Client(int argc, char ** argv)
    : CommandLine(argc, argv),
      crypto(QSslSocket::SslClientMode),
      loginSuccess(false),
      registerMode(false)
{
}

Client::~Client()
{
    shutdown();
}

/* public slots */
void Client::catbomb()
{
    if(loginSuccess)
    {
        qCritical() << tr("You have been bombarded by a cute cat.");
#pragma message(NOT_M_CONST)
        qout.printLine(QStringLiteral("TURKEY TROTS TO WATER"
                                      " GG"
                                      " FROM CINCPAC ACTION COM THIRD FLEET INFO COMINCH CTF SEVENTY-SEVEN X"
                                      " WHERE IS RPT WHERE IS TASK FORCE THIRTY FOUR"
                                      " RR"
                                      " THE WORLD WONDERS"),
                       Ecma(255,128,192), Ecma(255,255,255,true));
        loginSuccess = false;
    }
    else
    {
        qWarning() << tr("Failed to establish connection, check your username,"
                         "password and server status.");
        displayPrompt();
    }
    shutdown();
}

void Client::displayPrompt()
{
#if 0
    qInfo() << tr("田中飞妈") << 114514;
#endif
    if(passwordMode != password::normal)
        return;
    if(!loginSuccess)
        qout << "WAClient$ ";
    else
        qout << clientName << "@" << serverName << "$ ";
}

bool Client::parseSpec(const QStringList &cmdParts)
{
    if(cmdParts.length() > 0)
    {
        QString primary = cmdParts[0];
        if(passwordMode != password::normal)
        {
            QString password = primary;
            QByteArray salt = clientName.toUtf8().append(
                        settings->value("salt",
                                        defaultSalt).toByteArray());
            if(passwordMode == password::confirm)
            {
                QByteArray shadow1 = QPasswordDigestor::deriveKeyPbkdf2(
                            QCryptographicHash::Blake2s_256,
                            password.toUtf8(), salt, 8, 255);
                if(shadow1 != shadow)
                {
                    emit turnOnEchoing();
                    qWarning() << tr("Password does not match!");
                    passwordMode = password::normal;
                    return true;
                }
            }
            else
            {
                shadow = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Blake2s_256,
                                                            password.toUtf8(), salt, 8, 255);
            }
            if(passwordMode != password::registering)
            {
                emit turnOnEchoing();

                auto configuration = QSslConfiguration::defaultDtlsConfiguration();
                configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
                crypto.setPeer(address, port);
                crypto.setDtlsConfiguration(configuration);
                connect(&crypto, &QDtls::handshakeTimeout, this, &Client::handshakeTimeout);
                connect(&crypto, &QDtls::pskRequired, this, &Client::pskRequired);
                socket.connectToHost(address.toString(), port);
                if(!socket.waitForConnected(8000))
                {
                    qWarning() << tr("Failed to connect to server")
                               << address.toString()
                               << tr("port") << port;
                    passwordMode = password::normal;
                    return true;
                }
                connect(&socket, &QUdpSocket::readyRead, this, &Client::readyRead);
                startHandshake();
                passwordMode = password::normal;
            }
            else
            {
                qInfo() << tr("Confirm Password:");
                passwordMode = password::confirm;
            }
            return true;
        }

        /* aliases */
        QMap<QString, QString> aliases;
        aliases["cn"] = "connect";
        aliases["dc"] = "disconnect";
        aliases["reg"] = "register";

        if(aliases.contains(primary))
        {
            primary = aliases[primary];
        }
        /* end aliases */

        if(primary.compare("connect", Qt::CaseInsensitive) == 0
                || primary.compare("register", Qt::CaseInsensitive) == 0)
        {
            retransmitTimes = 0;
            registerMode = primary.compare("register", Qt::CaseInsensitive) == 0;
            if(cmdParts.length() < 4)
            {
                if(registerMode)
                    qInfo() << tr("Usage: register [ip] [port] [username]");
                else
                    qInfo() << tr("Usage: connect [ip] [port] [username]");
                return true;
            }
            else
            {
                if(crypto.isConnectionEncrypted())
                {
                    qInfo() << tr("Already connected, please shut down first.");
                    return true;
                }
                else
                {
                    address = QHostAddress(cmdParts[1]);
                    if(address.isNull())
                    {
                        qWarning() << tr("IP isn't valid");
                        return true;
                    }
                    port = QString(cmdParts[2]).toInt();
                    if(port < 1024 || port > 49151)
                    {
                        qWarning() << tr("Port isn't valid, it must fall between 1024 and 49151");
                        return true;
                    }

                    clientName = cmdParts[3];
                    emit turnOffEchoing();
                    qInfo() << tr("Enter Password:");
                    passwordMode = registerMode ? password::registering : password::login;
                }
                return true;
            }
        }
        else if(primary.compare("disconnect", Qt::CaseInsensitive) == 0)
        {
            if(!crypto.isConnectionEncrypted())
                qInfo() << tr("Not under a valid connection.");
            else
            {
                const qint64 written = crypto.writeDatagramEncrypted(&socket, "LOGOUT");

                if (written <= 0) {
                    qCritical() << clientName << tr(": failed to send logout attmpt -")
                                << crypto.dtlsErrorString();
                }
                loginSuccess = false;
                qInfo() << tr("Attempting to disconnect...");
            }
            return true;
        }
        return false;
    }
    return false;
}

void Client::serverResponse(const QString &clientInfo, const QByteArray &datagram,
                            const QByteArray &plainText)
{
    Q_UNUSED(datagram)
#ifdef QT_DEBUG
    static const QString formatter = QStringLiteral("%1 received text: %2");

    const QString html = formatter.arg(clientInfo, QString::fromUtf8(plainText));
    qDebug() << html;
#else
    Q_UNUSED(clientInfo)
    Q_UNUSED(plainText)
#endif
}

void Client::update()
{
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
    qout.flush();
}

/* private slots */
void Client::handshakeTimeout()
{
    maxRetransmit = settings->value("client/maximum_retransmit",
                                    defaultMaxRetransmit).toInt();
    qDebug() << clientName << ": handshake timeout, trying to re-transmit";
    retransmitTimes++;
    if (!crypto.handleTimeout(&socket))
        qDebug() << clientName << ": failed to re-transmit -" << crypto.dtlsErrorString();
    if(retransmitTimes > maxRetransmit)
    {
        qWarning() << clientName << tr(": max restransmit time exceeded!");
        catbomb();
    }
}

void Client::pskRequired(QSslPreSharedKeyAuthenticator *auth)
{
    Q_ASSERT(auth);

    qDebug() << clientName << ": providing pre-shared key ...";
    serverName = auth->identityHint();
    if(registerMode)
    {
        auth->setIdentity("NEW_USER");
        auth->setPreSharedKey(QByteArrayLiteral("register"));
    }
    else
    {
        auth->setIdentity(clientName.toLatin1());
        //auth->setPreSharedKey(shadow);
        auth->setPreSharedKey(QByteArrayLiteral("register"));
    }
}

void Client::readyRead()
{
    if(socket.pendingDatagramSize() <= 0)
    {
        qDebug() << clientName << ": spurious read notification?";
        return;
    }

    QByteArray dgram(socket.pendingDatagramSize(), Qt::Uninitialized);
    const qint64 bytesRead = socket.readDatagram(dgram.data(), dgram.size());
    if (bytesRead <= 0)
    {
        qDebug() << clientName << ": spurious read notification?";
        return;
    }

    dgram.resize(bytesRead);

    if (crypto.isConnectionEncrypted())
    {
        const QByteArray plainText = crypto.decryptDatagram(&socket, dgram);
        if (plainText.size())
        {
            serverResponse(clientName, dgram, plainText);
            return;
        }

        if (crypto.dtlsError() == QDtlsError::RemoteClosedConnectionError)
        {
            qDebug() << clientName << ": shutdown alert received";
            socket.close();
            if(loginSuccess)
                catbomb();
            else
            {
                shutdown();
                qInfo() << tr("Remote disconnected.");
            }
            return;
        }
        qDebug() << clientName << ": zero-length datagram received?";
    }
    else
    {
        if (!crypto.doHandshake(&socket, dgram))
        {
            qDebug() << clientName << ": handshake error -"
                     << crypto.dtlsErrorString();
            return;
        }
        if (crypto.isConnectionEncrypted())
        {
            qDebug() << clientName << ": encrypted connection established!";
            loginSuccess = true;

            QString shadowstring = QString(shadow.toHex()).toLatin1();
            if(registerMode)
            {
                static const QString message = QStringLiteral("REG %1 SHADOW %2");
                const qint64 written = crypto.writeDatagramEncrypted(&socket,
                                                                     message
                                                                     .arg(clientName, shadowstring)
                                                                     .toLatin1());
                if (written <= 0)
                {
                    qCritical() << clientName << tr(": register failed -")
                                << crypto.dtlsErrorString();
                }
            }
            else
            {
                static const QString message = QStringLiteral("LOGIN %1 SHADOW %2");
                const qint64 written = crypto.writeDatagramEncrypted(&socket,
                                                                     message
                                                                     .arg(clientName, shadowstring)
                                                                     .toLatin1());
                if (written <= 0)
                {
                    qCritical() << clientName << tr(": login failed -")
                                << crypto.dtlsErrorString();
                }
            }
        }
        else
        {
            qDebug() << clientName << ": continuing with handshake...";
        }
    }
}

void Client::shutdown()
{
    if(crypto.isConnectionEncrypted())
    {
        if(!crypto.shutdown(&socket))
        {
            qDebug() << clientName << ": shutdown socket failed!";
        }
    }
    if(crypto.handshakeState() == QDtls::HandshakeInProgress)
    {
        if(!crypto.abortHandshake(&socket))
        {
            qDebug() << crypto.dtlsErrorString();
        }
    }
    if(socket.isValid())
    {
        socket.close();
    }
    disconnect(&socket, &QUdpSocket::readyRead, this, &Client::readyRead);
    disconnect(&crypto, &QDtls::handshakeTimeout, this, &Client::handshakeTimeout);
    disconnect(&crypto, &QDtls::pskRequired, this, &Client::pskRequired);
}

void Client::startHandshake()
{
    if (socket.state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << clientName << ": connecting UDP socket first ...";
        connect(&socket, &QAbstractSocket::connected, this, &Client::udpSocketConnected);
        return;
    }

    if (!crypto.doHandshake(&socket))
    {
        qDebug() << clientName << ": failed to start a handshake -" << crypto.dtlsErrorString();
    }
    else
        qDebug() << clientName << ": starting a handshake ...";
}

void Client::udpSocketConnected()
{
    qDebug() << clientName << ": UDP socket is now in ConnectedState, continue with handshake ...";
    startHandshake();

    retransmitTimes = 0;
}

const QStringList Client::getCommandsSpec()
{
    QStringList result = QStringList();
    result.append(getCommands());
    result.append({"disconnect", "connect", "register"});
    result.sort(Qt::CaseInsensitive);
    return result;
}

const QStringList Client::getValidCommands()
{
    QStringList result = QStringList();
    result.append(getCommands());
    if(crypto.isConnectionEncrypted())
        result.append("disconnect");
    else
    {
        result.append("connect");
        result.append("register");
    }
    result.sort(Qt::CaseInsensitive);
    return result;
}

void Client::exitGraceSpec()
{
    shutdown();
}
