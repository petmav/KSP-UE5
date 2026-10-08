using System.Runtime.InteropServices;
using Unity;

namespace System.Reflection.Emit;

[Serializable]
[ComVisible(true)]
public struct StringToken
{
	private int DummyAddedByUnityProfileStubber;

	public int Token
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public bool Equals(StringToken obj)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator ==(StringToken a, StringToken b)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator !=(StringToken a, StringToken b)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
