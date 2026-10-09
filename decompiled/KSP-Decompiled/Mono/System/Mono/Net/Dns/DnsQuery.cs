using System;

namespace Mono.Net.Dns;

internal class DnsQuery : DnsPacket
{
	public DnsQuery(string name, DnsQType qtype, DnsQClass qclass)
	{
		if (string.IsNullOrEmpty(name))
		{
			throw new ArgumentNullException("name");
		}
		int encodedLength = DnsUtil.GetEncodedLength(name);
		if (encodedLength == -1)
		{
			throw new ArgumentException("Invalid DNS name", "name");
		}
		encodedLength += 16;
		packet = new byte[encodedLength];
		header = new DnsHeader(packet, 0);
		position = 12;
		WriteDnsName(name);
		WriteUInt16((ushort)qtype);
		WriteUInt16((ushort)qclass);
		base.Header.QuestionCount = 1;
		base.Header.IsQuery = true;
		base.Header.RecursionDesired = true;
	}
}
