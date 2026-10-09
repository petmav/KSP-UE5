using System.ComponentModel;
using System.Globalization;

namespace System.Configuration;

public class TimeSpanSecondsConverter : ConfigurationConverterBase
{
	public override object ConvertFrom(ITypeDescriptorContext ctx, CultureInfo ci, object data)
	{
		if (!(data is string))
		{
			throw new ArgumentException("data");
		}
		if (!long.TryParse((string)data, out var result))
		{
			throw new ArgumentException("data");
		}
		return TimeSpan.FromSeconds(result);
	}

	public override object ConvertTo(ITypeDescriptorContext ctx, CultureInfo ci, object value, Type type)
	{
		if (value.GetType() != typeof(TimeSpan))
		{
			throw new ArgumentException();
		}
		return ((long)((TimeSpan)value).TotalSeconds).ToString();
	}
}
