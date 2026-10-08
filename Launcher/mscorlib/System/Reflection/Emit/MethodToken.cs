using System.Runtime.InteropServices;
using Unity;

namespace System.Reflection.Emit;

[Serializable]
[ComVisible(true)]
public struct MethodToken
{
	public static readonly MethodToken Empty;

	public int Token
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public bool Equals(MethodToken obj)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator ==(MethodToken a, MethodToken b)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator !=(MethodToken a, MethodToken b)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
