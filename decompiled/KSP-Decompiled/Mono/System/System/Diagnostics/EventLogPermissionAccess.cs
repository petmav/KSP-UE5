namespace System.Diagnostics;

[Flags]
public enum EventLogPermissionAccess
{
	None = 0,
	[Obsolete]
	Browse = 2,
	[Obsolete]
	Instrument = 6,
	[Obsolete]
	Audit = 0xA,
	Write = 0x10,
	Administer = 0x30
}
