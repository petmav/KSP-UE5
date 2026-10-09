using System;

namespace Mono.Net.Dns;

internal class DnsResourceRecord
{
	private string name;

	private DnsType type;

	private DnsClass klass;

	private int ttl;

	private ushort rdlength;

	private ArraySegment<byte> m_rdata;

	public string Name => name;

	public DnsType Type => type;

	public DnsClass Class => klass;

	public int Ttl => ttl;

	public ArraySegment<byte> Data => m_rdata;

	internal DnsResourceRecord()
	{
	}

	internal void CopyFrom(DnsResourceRecord rr)
	{
		name = rr.name;
		type = rr.type;
		klass = rr.klass;
		ttl = rr.ttl;
		rdlength = rr.rdlength;
		m_rdata = rr.m_rdata;
	}

	internal static DnsResourceRecord CreateFromBuffer(DnsPacket packet, int size, ref int offset)
	{
		string text = packet.ReadName(ref offset);
		DnsType dnsType = (DnsType)packet.ReadUInt16(ref offset);
		DnsClass dnsClass = (DnsClass)packet.ReadUInt16(ref offset);
		int num = packet.ReadInt32(ref offset);
		ushort num2 = packet.ReadUInt16(ref offset);
		DnsResourceRecord dnsResourceRecord = new DnsResourceRecord();
		dnsResourceRecord.name = text;
		dnsResourceRecord.type = dnsType;
		dnsResourceRecord.klass = dnsClass;
		dnsResourceRecord.ttl = num;
		dnsResourceRecord.rdlength = num2;
		dnsResourceRecord.m_rdata = new ArraySegment<byte>(packet.Packet, offset, num2);
		offset += num2;
		if (dnsClass == DnsClass.Internet)
		{
			switch (dnsType)
			{
			case DnsType.A:
				dnsResourceRecord = new DnsResourceRecordA(dnsResourceRecord);
				break;
			case DnsType.AAAA:
				dnsResourceRecord = new DnsResourceRecordAAAA(dnsResourceRecord);
				break;
			case DnsType.CNAME:
				dnsResourceRecord = new DnsResourceRecordCName(dnsResourceRecord);
				break;
			case DnsType.PTR:
				dnsResourceRecord = new DnsResourceRecordPTR(dnsResourceRecord);
				break;
			}
		}
		return dnsResourceRecord;
	}

	public override string ToString()
	{
		return $"Name: {name}, Type: {type}, Class: {klass}, Ttl: {ttl}, Data length: {Data.Count}";
	}
}
