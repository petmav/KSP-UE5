namespace Mono.Net.Dns;

internal class DnsQuestion
{
	private string name;

	private DnsQType type;

	private DnsQClass _class;

	public string Name => name;

	public DnsQType Type => type;

	public DnsQClass Class => _class;

	internal DnsQuestion()
	{
	}

	internal int Init(DnsPacket packet, int offset)
	{
		name = packet.ReadName(ref offset);
		type = (DnsQType)packet.ReadUInt16(ref offset);
		_class = (DnsQClass)packet.ReadUInt16(ref offset);
		return offset;
	}

	public override string ToString()
	{
		return $"Name: {Name} Type: {Type} Class: {Class}";
	}
}
