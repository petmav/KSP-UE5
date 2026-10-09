using System.Runtime.InteropServices;
using Unity;

namespace System.Reflection.Emit;

[ComVisible(true)]
public struct SignatureToken
{
	public static readonly SignatureToken Empty;

	public int Token
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public bool Equals(SignatureToken obj)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator ==(SignatureToken a, SignatureToken b)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator !=(SignatureToken a, SignatureToken b)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
