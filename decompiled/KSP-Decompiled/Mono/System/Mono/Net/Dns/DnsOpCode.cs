using System;

namespace Mono.Net.Dns;

internal enum DnsOpCode : byte
{
	Query = 0,
	[Obsolete]
	IQuery = 1,
	Status = 2,
	Notify = 4,
	Update = 5
}
