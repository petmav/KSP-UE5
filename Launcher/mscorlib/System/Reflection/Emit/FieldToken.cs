using System.Runtime.InteropServices;
using Unity;

namespace System.Reflection.Emit;

[Serializable]
[ComVisible(true)]
public struct FieldToken
{
	public static readonly FieldToken Empty;

	public int Token
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public bool Equals(FieldToken obj)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator ==(FieldToken a, FieldToken b)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator !=(FieldToken a, FieldToken b)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
