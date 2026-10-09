namespace Mono.Net.Dns;

internal class DnsResourceRecordPTR : DnsResourceRecord
{
	private string dname;

	public string DName => dname;

	internal DnsResourceRecordPTR(DnsResourceRecord rr)
	{
		CopyFrom(rr);
		int offset = rr.Data.Offset;
		dname = DnsPacket.ReadName(rr.Data.Array, ref offset);
	}

	public override string ToString()
	{
		return base.ToString() + " DNAME: " + dname.ToString();
	}
}
