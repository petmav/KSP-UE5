using System.Runtime.InteropServices;
using Unity;

namespace System.Reflection.Emit;

[Serializable]
[ComVisible(true)]
public struct EventToken
{
	public static readonly EventToken Empty;

	public int Token
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public bool Equals(EventToken obj)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator ==(EventToken a, EventToken b)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator !=(EventToken a, EventToken b)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
