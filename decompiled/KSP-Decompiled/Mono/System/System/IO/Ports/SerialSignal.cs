namespace System.IO.Ports;

internal enum SerialSignal
{
	None = 0,
	Cd = 1,
	Cts = 2,
	Dsr = 4,
	Dtr = 8,
	Rts = 0x10
}
