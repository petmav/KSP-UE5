namespace Mono.Net.Dns;

internal enum DnsQClass : ushort
{
	Internet = 1,
	IN = 1,
	CSNET = 2,
	CS = 2,
	CHAOS = 3,
	CH = 3,
	Hesiod = 4,
	HS = 4,
	None = 254,
	Any = 255
}
