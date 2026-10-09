using System.Drawing.Text;
using Unity;

namespace System.Drawing;

public sealed class StringFormat : MarshalByRefObject, ICloneable, IDisposable
{
	public StringAlignment Alignment
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(StringAlignment);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int DigitSubstitutionLanguage
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public StringDigitSubstitute DigitSubstitutionMethod
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(StringDigitSubstitute);
		}
	}

	public StringFormatFlags FormatFlags
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(StringFormatFlags);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static StringFormat GenericDefault
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static StringFormat GenericTypographic
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public HotkeyPrefix HotkeyPrefix
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(HotkeyPrefix);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public StringAlignment LineAlignment
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(StringAlignment);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public StringTrimming Trimming
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(StringTrimming);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public StringFormat()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public StringFormat(StringFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public StringFormat(StringFormatFlags options)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public StringFormat(StringFormatFlags options, int language)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public float[] GetTabStops(out float firstTabOffset)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void SetDigitSubstitution(int language, StringDigitSubstitute substitute)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetMeasurableCharacterRanges(CharacterRange[] ranges)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetTabStops(float firstTabOffset, float[] tabStops)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
