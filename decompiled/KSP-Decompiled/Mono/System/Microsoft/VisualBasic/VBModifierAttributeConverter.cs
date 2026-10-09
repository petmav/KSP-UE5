using System;
using System.ComponentModel;
using System.Globalization;

namespace Microsoft.VisualBasic;

internal abstract class VBModifierAttributeConverter : TypeConverter
{
	protected abstract object[] Values { get; }

	protected abstract string[] Names { get; }

	protected abstract object DefaultValue { get; }

	public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
	{
		if (!(sourceType == typeof(string)))
		{
			return base.CanConvertFrom(context, sourceType);
		}
		return true;
	}

	public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
	{
		if (value is string value2)
		{
			string[] names = Names;
			for (int i = 0; i < names.Length; i++)
			{
				if (names[i].Equals(value2, StringComparison.OrdinalIgnoreCase))
				{
					return Values[i];
				}
			}
		}
		return DefaultValue;
	}

	public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
	{
		if (destinationType == null)
		{
			throw new ArgumentNullException("destinationType");
		}
		if (destinationType == typeof(string))
		{
			object[] values = Values;
			for (int i = 0; i < values.Length; i++)
			{
				if (values[i].Equals(value))
				{
					return Names[i];
				}
			}
			return "(unknown)";
		}
		return base.ConvertTo(context, culture, value, destinationType);
	}

	public override bool GetStandardValuesExclusive(ITypeDescriptorContext context)
	{
		return true;
	}

	public override bool GetStandardValuesSupported(ITypeDescriptorContext context)
	{
		return true;
	}

	public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context)
	{
		return new StandardValuesCollection(Values);
	}
}
