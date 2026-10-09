using System;

namespace Mono.Unix.Native;

[Map("struct linger")]
[CLSCompliant(false)]
public struct Linger
{
	public int l_onoff;

	public int l_linger;

	public override string ToString()
	{
		return $"{l_onoff}, {l_linger}";
	}
}
