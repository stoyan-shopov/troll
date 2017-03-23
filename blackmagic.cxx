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

QString xxx;
QVector<QByteArray> Blackmagic::readGdbServerResponse(const QByteArray & request)
{
QByteArray response, packet;
QVector<QByteArray> reply_packets;
	if (port->write(request) == -1)
		Util::panic();
	if (!port->waitForBytesWritten(1000))
		Util::panic();
	while (1)
	{
		if (response.isEmpty() && !port->waitForReadyRead(1000))
			return reply_packets;
		response += port->readAll();
		if (!(packet = GdbRemote::extractPacket(response)).isEmpty())
		{
			qDebug() << packet;
			reply_packets.push_back(packet);
		}
		port->write("+");
		if (!port->waitForBytesWritten(1000))
			Util::panic();
		if (GdbRemote::isOkResponse(packet) || GdbRemote::isEmptyResponse(packet))
			break;
	}
	return reply_packets;
}

bool Blackmagic::connect()
{
QByteArray packet = GdbRemote::monitorRequest("s");
int i;
QVector<QByteArray> r;

	if (!port->setDataTerminalReady(true)) Util::panic();
	while (1)
	{
		r = readGdbServerResponse("+");
		if (!r.size())
			r = readGdbServerResponse(packet);
		if (!GdbRemote::isOkResponse(r.at(r.size() - 1)))
			Util::panic();
		if (r.size() != 1)
			break;
	}
	//r = readGdbServerResponse(packet);
	for (i = 0; i < r.size() - 1; i ++)
	{
		auto s = GdbRemote::packetData(r[i]);
		if (s[0] == 'O')
		{
			qDebug() << QByteArray::fromHex(s.mid(1));
		}
	}
	Util::panic();
}
