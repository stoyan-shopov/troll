/*
Copyright (c) 2019 stoyan shopov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <QDebug>
#include <QMessageBox>

#include "gdbserver.hxx"
#include "gdb-remote.hxx"

GdbServer::GdbServer()
{
QRegularExpression rx("\\$(.*)#(..)", QRegularExpression::InvertedGreedinessOption);

	if (!gdb_tcpserver.listen(QHostAddress::Any, 1122))
	{
		QMessageBox::critical(0, "Cannot listen on gdbserver port", "Error opening a gdbserver port for listening for incoming gdb connections");
		return;
	}
	connect(& gdb_tcpserver, SIGNAL(newConnection()), this, SLOT(newConneciton()));
}

void GdbServer::handleGdbPacket(const QByteArray &packet)
{
	auto pd = GdbRemote::packetData(packet);
	if (pd.startsWith("qSupported"))
		sendGdbReply(GdbRemote::emptyResponsePacket());
	else if (pd.startsWith("!"))
		/* Enable extended mode. This is always enabled */
		sendGdbReply(GdbRemote::okResponsePacket());
	else if (pd.startsWith("?"))
		/* Indicate the reason the target halted */
		sendGdbReply(GdbRemote::stopReplySignalNumberPacket(POSIX_SIGTRAP));
	else if (pd.startsWith("g"))
		/* Read general registers */
		sendGdbReply(GdbRemote::errorReplyPacket(255));
	else
	{
		qDebug() << "Unhandled gdb remote packet:" << packet;
		sendGdbReply(GdbRemote::emptyResponsePacket());
	}
}

void GdbServer::newConneciton(void)
{
	qDebug() << "gdb client connected";
	gdb_client_socket = gdb_tcpserver.nextPendingConnection();
	connect(gdb_client_socket, &QTcpSocket::disconnected, [&] { gdb_client_socket->disconnect(); gdb_client_socket = 0; qDebug() << "gdb client disconnected"; });
	connect(gdb_client_socket, SIGNAL(readyRead()), this, SLOT(gdbClientSocketReadyRead()));
}

void GdbServer::gdbClientSocketReadyRead(void)
{
	incoming_data += gdb_client_socket->readAll();
	QByteArray packet;
	while ((packet = GdbRemote::extractPacket(incoming_data)).length())
	{
		qDebug() << "Received gdb packet:" << packet;
		if (!GdbRemote::isValidPacket(packet))
		{
			qDebug() << "Invalid gdb packet received:" << packet;
			gdb_client_socket->write("-");
		}
		else
		{
			gdb_client_socket->write("+");
			handleGdbPacket(packet);
		}
	}
}
