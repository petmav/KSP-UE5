using System;

namespace Mono.Net;

internal struct CFRange
{
	public IntPtr Location;

	public IntPtr Length;

	public CFRange(int loc, int len)
	{
		Location = (IntPtr)loc;
		Length = (IntPtr)len;
	}
}
