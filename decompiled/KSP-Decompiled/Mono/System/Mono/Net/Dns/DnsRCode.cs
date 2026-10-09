namespace Mono.Net.Dns;

internal enum DnsRCode : ushort
{
	NoError = 0,
	FormErr = 1,
	ServFail = 2,
	NXDomain = 3,
	NotImp = 4,
	Refused = 5,
	YXDomain = 6,
	YXRRSet = 7,
	NXRRSet = 8,
	NotAuth = 9,
	NotZone = 10,
	BadVers = 16,
	BadSig = 16,
	BadKey = 17,
	BadTime = 18,
	BadMode = 19,
	BadName = 20,
	BadAlg = 21,
	BadTrunc = 22
}
