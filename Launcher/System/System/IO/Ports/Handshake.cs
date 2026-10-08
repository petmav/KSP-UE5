namespace System.IO.Ports;

public enum Handshake
{
	None = 0,
	RequestToSend = 2,
	RequestToSendXOnXOff = 3,
	XOnXOff = 1
}
