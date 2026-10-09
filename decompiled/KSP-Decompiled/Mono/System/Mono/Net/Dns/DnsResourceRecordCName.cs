namespace Mono.Net.Dns;

internal class DnsResourceRecordCName : DnsResourceRecord
{
	private string cname;

	public string CName => cname;

	internal DnsResourceRecordCName(DnsResourceRecord rr)
	{
		CopyFrom(rr);
		int offset = rr.Data.Offset;
		cname = DnsPacket.ReadName(rr.Data.Array, ref offset);
	}

	public override string ToString()
	{
		return base.ToString() + " CNAME: " + cname.ToString();
	}
}
