using System.Runtime.InteropServices;
using Unity;

namespace System.Reflection.Emit;

[Serializable]
[ComVisible(true)]
public struct ParameterToken
{
	public static readonly ParameterToken Empty;

	public int Token
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public bool Equals(ParameterToken obj)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator ==(ParameterToken a, ParameterToken b)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator !=(ParameterToken a, ParameterToken b)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
