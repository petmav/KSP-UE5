using System.Runtime.InteropServices;

namespace System.IO.Ports;

[StructLayout(LayoutKind.Sequential)]
internal class DCB
{
	public int dcb_length;

	public int baud_rate;

	public int flags;

	public short w_reserved;

	public short xon_lim;

	public short xoff_lim;

	public byte byte_size;

	public byte parity;

	public byte stop_bits;

	public byte xon_char;

	public byte xoff_char;

	public byte error_char;

	public byte eof_char;

	public byte evt_char;

	public short w_reserved1;

	private const int fOutxCtsFlow = 4;

	private const int fOutX = 256;

	private const int fInX = 512;

	private const int fRtsControl2 = 8192;

	public void SetValues(int baud_rate, Parity parity, int byte_size, StopBits sb, Handshake hs)
	{
		switch (sb)
		{
		case StopBits.One:
			stop_bits = 0;
			break;
		case StopBits.OnePointFive:
			stop_bits = 1;
			break;
		case StopBits.Two:
			stop_bits = 2;
			break;
		}
		this.baud_rate = baud_rate;
		this.parity = (byte)parity;
		this.byte_size = (byte)byte_size;
		flags &= -8965;
		switch (hs)
		{
		case Handshake.XOnXOff:
			flags |= 768;
			break;
		case Handshake.RequestToSend:
			flags |= 8196;
			break;
		case Handshake.RequestToSendXOnXOff:
			flags |= 8964;
			break;
		case Handshake.None:
			break;
		}
	}
}
