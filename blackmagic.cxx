/*
Copyright (c) 2017 stoyan shopov

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
#include "blackmagic.hxx"

#include "gdb-remote.hxx"


void Blackmagic::putPacket(const QByteArray &request)
{
	port->write(request);
	port->waitForBytesWritten(1000);
	while (getChar() != '+')
		;//Util::panic();
}

QByteArray Blackmagic::getPacket()
{
QByteArray packet("$");
	while (getChar() != '$')
		;
	while (1)
	{
		packet += getChar();
		if (packet[packet.length() - 1] == '#')
			break;
	}
	packet += getChar();
	packet += getChar();
	port->write("+");
	port->waitForBytesWritten(1000);
	return packet;
}

char Blackmagic::getChar()
{
char c;
	while (!port->bytesAvailable())
		port->waitForReadyRead(10);
	if (!port->getChar(& c))
		Util::panic();
	return c;
}

bool Blackmagic::connect()
{
QByteArray packet = GdbRemote::monitorRequest("s");
int i;
QVector<QByteArray> r;

	if (!port->setDataTerminalReady(true))
		Util::panic();
	port->write("+++");
	port->waitForBytesWritten(1000);
	port->readAll();
	qDebug() << "--------------> +++";
	do
		port->waitForReadyRead(1000);
	while (port->readAll() > 0);
	
	putPacket(packet);
	do
		r.push_back(getPacket());
	while (!GdbRemote::isOkResponse(r.back()));

	for (i = 0; i < r.size() - 1; i ++)
	{
		auto s = GdbRemote::packetData(r[i]);
		if (s[0] == 'O')
		{
			qDebug() << QByteArray::fromHex(s.mid(1));
		}
	}
	qDebug() << r[r.length() - 1];
	Util::panic();
}
