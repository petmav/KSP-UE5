namespace System.IO.Ports;

public enum SerialError
{
	Frame = 8,
	Overrun = 2,
	RXOver = 1,
	RXParity = 4,
	TXFull = 256
}
