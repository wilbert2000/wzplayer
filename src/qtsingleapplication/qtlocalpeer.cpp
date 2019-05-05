/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qtlocalpeer.h"
#include <QCoreApplication>
#include <QDataStream>
#include <QTime>

#if defined(Q_OS_WIN)
#include <QLibrary>
#include <qt_windows.h>
typedef BOOL(WINAPI*PProcessIdToSessionId)(DWORD,DWORD*);
static PProcessIdToSessionId pProcessIdToSessionId = 0;
#endif

#if defined(Q_OS_UNIX)
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#endif

namespace QtLP_Private {
#include "qtlockedfile.cpp"
#if defined(Q_OS_WIN)
#include "qtlockedfile_win.cpp"
#else
#include "qtlockedfile_unix.cpp"
#endif
}

#include "wzdebug.h"
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QDir>


LOG4QT_DECLARE_STATIC_LOGGER(logger, QtLocalPeer)

const char* QtLocalPeer::ack = "ack";
const char* QtLocalPeer::squit = "squit";

static QString getUID() {

    QString uid;

#if defined(Q_OS_WIN)
    if (!pProcessIdToSessionId) {
        QLibrary lib("kernel32");
        pProcessIdToSessionId =
                (PProcessIdToSessionId)lib.resolve("ProcessIdToSessionId");
    }
    if (pProcessIdToSessionId) {
        DWORD sessionId = 0;
        pProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
        uid = QString::number(sessionId);
    } else {
        uid = "noid";
    }
#else
    uid = QString::number(::getuid());
#endif

    return uid;
}

QtLocalPeer::QtLocalPeer(QObject* parent, const QString &appId) :
    QObject(parent) {

    // Use data dir instead of temp?
    socketDir = QDir::tempPath();
    if (!socketDir.endsWith("/")) {
        socketDir += "/";
    }
    socketDir += appId + "-" + getUID();
    if (!QDir().mkpath(socketDir)) {
        WZE << "Failed to create socket directory" << socketDir
            << strerror(errno);
    }

    clientSocketName = socketDir + "/client-" + QString::number(::getpid());
    serverSocketName = socketDir + "/server";

    // Open server lock file in unlocked state, will be locked in isClient()
    serverLockFile.setFileName(serverSocketName + "-lockfile");
    serverLockFile.open(QIODevice::ReadWrite);

    server = new QLocalServer(this);
    connect(server, &QLocalServer::newConnection,
            this, &QtLocalPeer::receiveConnection);
}

QtLocalPeer::~QtLocalPeer() {
    closeServer();
}

bool QtLocalPeer::listen(const QString& name) {

    if (server->isListening()) {
        WZT << "Closing" << server->serverName();
        server->close();
    }
    bool res = server->listen(name);

#if defined(Q_OS_UNIX)
    // Workaround adress in use. Remove the socket file.
    if (!res && server->serverError() == QAbstractSocket::AddressInUseError) {
        QFile::remove(name);
        res = server->listen(name);
    }
#endif

    if (res) {
        WZT << "Started listening on socket" << name;
    } else {
        WZE << "Failed to listen on socket" << name << server->errorString();
    }
    return res;
}

bool QtLocalPeer::isClient() {

    if (serverLockFile.isLocked()) {
        // Is server
        return false;
    }

    // Can we get the server lock?
    if (serverLockFile.lock(QtLP_Private::QtLockedFile::WriteLock, false)) {
        // Start listening on server socket
        listen(serverSocketName);
        // Is server
        return false;
    }

    // Start listening on client socket
    if (!server->isListening()) {
        listen(clientSocketName);
    }
    // Is client
    return true;
}

bool QtLocalPeer::sendMsg(const QString& toSocket,
                          const QString &message,
                          int timeout) {

    // Connect
    QLocalSocket socket;
    bool connOk = false;
    for(int i = 0; i < 2; i++) {
        // Try twice, in case the other instance is just starting up
        socket.connectToServer(toSocket);
        connOk = socket.waitForConnected(timeout/2);
        if (connOk || i)
            break;
        int ms = 250;
#if defined(Q_OS_WIN)
        Sleep(DWORD(ms));
#else
        struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
        nanosleep(&ts, NULL);
#endif
    }

    if (!connOk) {
        WZD << "Failed to connect to" << toSocket << socket.errorString();
        return false;
    }

    // Write message
    QByteArray uMsg(message.toUtf8());
    QDataStream ds(&socket);
    ds.writeBytes(uMsg.constData(), uMsg.size());
    bool res = socket.waitForBytesWritten(timeout);

    // Read ack
    if (res) {
        res &= socket.waitForReadyRead(timeout);
        if (res) {
            res &= (socket.read(qstrlen(ack)) == ack);
        }
    }

    if (res) {
        WZT << "Message" << message << "written to socket" << toSocket;
    } else {
        WZW << "Failed to write message to" << toSocket;
    }
    return res;
}

bool QtLocalPeer::sendMessage(const QString &message, int timeout) {

    if (!isClient()) {
        WZW << "Ignoring sendMessage() by server" << message;
        return false;
    }
    return sendMsg(serverSocketName, message, timeout);
}

void QtLocalPeer::onSocketReadyRead() {

    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (socket) {
        QByteArray bytes = socket->read(qstrlen(ack));
        socket->disconnectFromServer();
        if (bytes == ack) {
            WZT << "Received ack on socket" << socket->serverName();
        } else {
            WZW << "Failed to receive ack. Instead received" << bytes
                << "from socket" << socket->serverName();
        }
        socket->deleteLater();
    }
}

void QtLocalPeer::onSocketConnected() {

    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (socket) {
        connect(socket, &QLocalSocket::readyRead,
                this, &QtLocalPeer::onSocketReadyRead);

        // Write msg to socket
        QString msg = socket->property("msg").toString();
        QByteArray uMsg(msg.toUtf8());
        QDataStream ds(socket);
        ds.writeBytes(uMsg.constData(), uMsg.size());
        WZT << "Connected to" << socket->serverName()
            << uMsg.size() << "bytes written";
    }
}

void QtLocalPeer::onSocketError(QLocalSocket::LocalSocketError socketError) {

    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (socket) {
        WZE << "Socket error" << socketError << socket->errorString()
            << "on socket" << socket->serverName();
        socket->deleteLater();
    } else {
        WZE << "Socket error" << socketError << "without socket";
    }
}

void QtLocalPeer::connectTo(const QString& toSocket,
                            const QString& message,
                            int timeout) {
    WZT << "Connecting to" << toSocket;

    QLocalSocket* socket = new QLocalSocket(this);
    socket->setProperty("msg", message);
    connect(socket, SIGNAL(error(QLocalSocket::LocalSocketError)),
            this, SLOT(onSocketError(QLocalSocket::LocalSocketError)));
    connect(socket, &QLocalSocket::connected,
            this, &QtLocalPeer::onSocketConnected);
    socket->connectToServer(toSocket);

    // Timeout socket
    QTimer::singleShot(timeout, socket, &QLocalSocket::deleteLater);
}

void QtLocalPeer::broadcastMessage(const QString &message, int timeout) {
    WZT << message << "timeout" << timeout;

    // Make sure we are listening
    isClient();
    if (message.isEmpty()) {
        return;
    }

    QDir dir(socketDir);
    dir.setFilter(QDir::Files | QDir::System);
    QStringList nameFilterList;
    nameFilterList << "server" << "client-*";
    dir.setNameFilters(nameFilterList);

    foreach(const QFileInfo& fi, dir.entryInfoList()) {
        if (fi.absoluteFilePath() != server->serverName()) {
            connectTo(fi.absoluteFilePath(), message, timeout);
        }
    }
}

void QtLocalPeer::receiveConnection() {

    QLocalSocket* socket = server->nextPendingConnection();
    if (!socket)
        return;

    // Wait message size available
    while (socket->bytesAvailable() < (int)sizeof(quint32))
        socket->waitForReadyRead();

    QDataStream ds(socket);
    QByteArray uMsg;
    // Get message size
    quint32 remaining;
    ds >> remaining;
    /// Get message
    uMsg.resize(remaining);
    int got = 0;
    char* uMsgBuf = uMsg.data();
    do {
        got = ds.readRawData(uMsgBuf, remaining);
        remaining -= got;
        uMsgBuf += got;
    } while (remaining && got >= 0 && socket->waitForReadyRead(2000));

    if (got < 0) {
        WZW << "Failed to read message" << socket->errorString();
        delete socket;
        return;
    }

    QString message(QString::fromUtf8(uMsg));
    socket->write(ack, qstrlen(ack));
    socket->waitForBytesWritten(1000);
    socket->waitForDisconnected(1000); // make sure client reads ack
    delete socket;

    if (message == squit) {
        WZTRACE("Received squit, picking up server role");
        isClient();
    } else {
        WZTRACE("emit messageReceived(\"" + message + "\")");
        emit messageReceived(message);
    }
}

void QtLocalPeer::closeServer() {

    if (!serverLockFile.isLocked()) {
        return;
    }

    // Shutdown server, so a remaining client can become server
    delete server;

    if (!serverLockFile.unlock()) {
        WZERROR("Failed to unlock lock file");
        return;
    }

    // Notify remaining client to pick up server role
    QDir dir(socketDir);
    dir.setFilter(QDir::Files | QDir::System);
    QStringList nameFilterList;
    nameFilterList << "client-*";
    dir.setNameFilters(nameFilterList);

    foreach(const QFileInfo& fi, dir.entryInfoList()) {
        if (sendMsg(fi.absoluteFilePath(), squit, 5000)) {
            break;
        } else {
            QFile file(fi.absoluteFilePath());
            if (file.remove()) {
                WZW << "Removed dead socket" << fi.absoluteFilePath();
            } else {
                WZE << "Failed to remove dead socket" << fi.absoluteFilePath()
                    << file.errorString();
            }
        }
    }
}
