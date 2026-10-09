using System.ComponentModel;
using System.ComponentModel.Design.Serialization;
using System.Globalization;

namespace System;

public class UriTypeConverter : TypeConverter
{
	private bool CanConvert(Type type)
	{
		if (type == typeof(string))
		{
			return true;
		}
		if (type == typeof(Uri))
		{
			return true;
		}
		return type == typeof(InstanceDescriptor);
	}

	public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
	{
		if (sourceType == null)
		{
			throw new ArgumentNullException("sourceType");
		}
		return CanConvert(sourceType);
	}

	public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
	{
		if (destinationType == null)
		{
			return false;
		}
		return CanConvert(destinationType);
	}

	public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (!CanConvertFrom(context, value.GetType()))
		{
			throw new NotSupportedException(global::Locale.GetText("Cannot convert from value."));
		}
		if (value is Uri)
		{
			return value;
		}
		if (value is string uriString)
		{
			return new Uri(uriString, UriKind.RelativeOrAbsolute);
		}
		if (value is InstanceDescriptor instanceDescriptor)
		{
			return instanceDescriptor.Invoke();
		}
		return base.ConvertFrom(context, culture, value);
	}

	public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
	{
		if (!CanConvertTo(context, destinationType))
		{
			throw new NotSupportedException(global::Locale.GetText("Cannot convert to destination type."));
		}
		Uri uri = value as Uri;
		if (uri != null)
		{
			if (destinationType == typeof(string))
			{
				return uri.ToString();
			}
			if (destinationType == typeof(Uri))
			{
				return uri;
			}
			if (destinationType == typeof(InstanceDescriptor))
			{
				return new InstanceDescriptor(typeof(Uri).GetConstructor(new Type[2]
				{
					typeof(string),
					typeof(UriKind)
				}), new object[2]
				{
					uri.ToString(),
					uri.IsAbsoluteUri ? UriKind.Absolute : UriKind.Relative
				});
			}
		}
		return base.ConvertTo(context, culture, value, destinationType);
	}

	public override bool IsValid(ITypeDescriptorContext context, object value)
	{
		if (value == null)
		{
			return false;
		}
		if (!(value is string))
		{
			return value is Uri;
		}
		return true;
	}
}
