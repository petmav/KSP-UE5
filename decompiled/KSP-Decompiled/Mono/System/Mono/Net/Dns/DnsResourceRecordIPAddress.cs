using System;
using System.Net;

namespace Mono.Net.Dns;

internal abstract class DnsResourceRecordIPAddress : DnsResourceRecord
{
	private IPAddress address;

	public IPAddress Address => address;

	internal DnsResourceRecordIPAddress(DnsResourceRecord rr, int address_size)
	{
		CopyFrom(rr);
		ArraySegment<byte> data = rr.Data;
		byte[] dst = new byte[address_size];
		Buffer.BlockCopy(data.Array, data.Offset, dst, 0, address_size);
		address = new IPAddress(dst);
	}

	public override string ToString()
	{
		return base.ToString() + " Address: " + address;
	}
}
