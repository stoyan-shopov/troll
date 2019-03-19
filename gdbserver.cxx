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

const QByteArray GdbServer::cortexmTargetDescriptionXml =
		"<?xml version=\"1.0\"?>"
		"<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
		"<target>"
		"  <architecture>arm</architecture>"
		"  <feature name=\"org.gnu.gdb.arm.m-profile\">"
		"    <reg name=\"r0\" bitsize=\"32\"/>"
		"    <reg name=\"r1\" bitsize=\"32\"/>"
		"    <reg name=\"r2\" bitsize=\"32\"/>"
		"    <reg name=\"r3\" bitsize=\"32\"/>"
		"    <reg name=\"r4\" bitsize=\"32\"/>"
		"    <reg name=\"r5\" bitsize=\"32\"/>"
		"    <reg name=\"r6\" bitsize=\"32\"/>"
		"    <reg name=\"r7\" bitsize=\"32\"/>"
		"    <reg name=\"r8\" bitsize=\"32\"/>"
		"    <reg name=\"r9\" bitsize=\"32\"/>"
		"    <reg name=\"r10\" bitsize=\"32\"/>"
		"    <reg name=\"r11\" bitsize=\"32\"/>"
		"    <reg name=\"r12\" bitsize=\"32\"/>"
		"    <reg name=\"sp\" bitsize=\"32\" type=\"data_ptr\"/>"
		"    <reg name=\"lr\" bitsize=\"32\" type=\"code_ptr\"/>"
		"    <reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>"
		"    <reg name=\"xpsr\" bitsize=\"32\"/>"
		"    <reg name=\"msp\" bitsize=\"32\" save-restore=\"no\" type=\"data_ptr\"/>"
		"    <reg name=\"psp\" bitsize=\"32\" save-restore=\"no\" type=\"data_ptr\"/>"
		"    <reg name=\"special\" bitsize=\"32\" save-restore=\"no\"/>"
		"  </feature>"
		"</target>"
		;


GdbServer::GdbServer(Target *target)
{
	this->target = target;
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
		sendGdbReply(GdbRemote::rawResponsePacket("qXfer:features:read+"));
	else if (pd.startsWith("!"))
		/* Enable extended mode. This is always enabled */
		sendGdbReply(GdbRemote::okResponsePacket());
	else if (pd.startsWith("?"))
		/* Indicate the reason the target halted */
		//sendGdbReply(GdbRemote::targetExitedExitcodeNumberPacket(0));
		sendGdbReply(GdbRemote::stopReplySignalNumberPacket(POSIX_SIGTRAP));
	else if (pd.startsWith("g"))
	{
		/* Read general registers */
		QByteArray registers;
		for (auto i = 0; i < 16; i++)
		{
			uint32_t r = target->readRawUncachedRegister(i);
			registers += QByteArray((const char *) & r, sizeof(uint32_t));
		}
		/*! \todo	Fix this. The number of registers' contents returned to
		 *		gdb must equal the number of registers from the target
		 * 		description xml document, but the last four registers
		 *		are not yet handled, and dummy values are being sent for
		 * 		them */
		sendGdbReply(GdbRemote::rawResponsePacket(registers.toHex() + /* xpsr */ "00" + /* msp*/ "00" + /* psp */ "00" +
			     /*! \todo 'special', I am not sure what this is, but I guess it is the 'CONTROL' register.
			      *	Look this up in the blackmagic sources */ "00"));
	}
	else if (pd.startsWith("m"))
	{
		/* Read memory */
		QRegularExpression rx("m(.+),(.+)");
		QRegularExpressionMatch match = rx.match(pd);
		if (!match.hasMatch())
			sendGdbReply(GdbRemote::errorReplyPacket(1));
		else
		{
			int address = match.captured(1).toUInt(0, 16), len = match.captured(2).toUInt(0, 16);
			QByteArray data = target->readBytes(address, len);
			sendGdbReply(GdbRemote::rawResponsePacket(data.toHex()));
		}
	}
	else if (pd.startsWith("qXfer:features:read:target.xml:"))
	{
		/* Access target description */
		QRegularExpression rx("qXfer:features:read:target.xml:(.+),(.+)");
		QRegularExpressionMatch match = rx.match(pd);
		if (!match.hasMatch())
			sendGdbReply(GdbRemote::errorReplyPacket(255));
		else
		{
			int start = match.captured(1).toUInt(0, 16), len = match.captured(2).toUInt(0, 16);
			if (start < cortexmTargetDescriptionXml.length())
				sendGdbReply(GdbRemote::rawResponsePacket(QByteArray("m") + cortexmTargetDescriptionXml.mid(start, len)));
			else if (len == cortexmTargetDescriptionXml.length())
				sendGdbReply(GdbRemote::rawResponsePacket("l"));
			else
				sendGdbReply(GdbRemote::errorReplyPacket(1));
		}
	}
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
