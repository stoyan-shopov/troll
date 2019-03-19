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

#ifndef GDBSERVER_HXX
#define GDBSERVER_HXX

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

#include "gdb-remote.hxx"
#include "target.hxx"

class GdbServer : public QObject
{
	Q_OBJECT
public:
	GdbServer(Target * target);

private:
	enum
	{
		/* Signal numbers for gdb stop reply packets. Looks like gdb uses the posix signal numbers */
		POSIX_SIGTRAP		= 5,
	};
	Target		* target;
	QTcpServer	gdb_tcpserver;
	QTcpSocket	* gdb_client_socket = 0;
	QByteArray	incoming_data;
	void		handleGdbPacket(const QByteArray& packet);
	void		sendGdbReply(const QByteArray& packet) { qDebug() << "Sending reply:" << packet; gdb_client_socket->write(packet); }
	static const QByteArray cortexmTargetDescriptionXml;
private slots:
	void newConneciton(void);
	void gdbClientSocketReadyRead(void);
};

#endif // GDBSERVER_HXX
