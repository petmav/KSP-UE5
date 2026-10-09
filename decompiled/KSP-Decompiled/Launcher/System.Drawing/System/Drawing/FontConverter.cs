using System.ComponentModel;
using Unity;

namespace System.Drawing;

public class FontConverter : TypeConverter
{
	public sealed class FontNameConverter : TypeConverter, IDisposable
	{
		public FontNameConverter()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}

		void IDisposable.Dispose()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public class FontUnitConverter : EnumConverter
	{
		public FontUnitConverter()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public FontConverter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
