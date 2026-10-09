using System.ComponentModel;
using Unity;

namespace System.Drawing.Printing;

[Serializable]
[TypeConverter(typeof(MarginsConverter))]
public class Margins : ICloneable
{
	public int Bottom
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int Left
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int Right
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int Top
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public Margins()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Margins(int left, int right, int top, int bottom)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static bool operator ==(Margins m1, Margins m2)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator !=(Margins m1, Margins m2)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
