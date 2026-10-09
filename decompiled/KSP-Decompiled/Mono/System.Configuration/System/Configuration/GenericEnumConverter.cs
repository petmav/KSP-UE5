using System.ComponentModel;
using System.Globalization;

namespace System.Configuration;

public sealed class GenericEnumConverter : ConfigurationConverterBase
{
	private Type typeEnum;

	public GenericEnumConverter(Type typeEnum)
	{
		if (typeEnum == null)
		{
			throw new ArgumentNullException("typeEnum");
		}
		this.typeEnum = typeEnum;
	}

	public override object ConvertFrom(ITypeDescriptorContext ctx, CultureInfo ci, object data)
	{
		if (data == null)
		{
			throw new ArgumentException();
		}
		return Enum.Parse(typeEnum, (string)data);
	}

	public override object ConvertTo(ITypeDescriptorContext ctx, CultureInfo ci, object value, Type type)
	{
		return value.ToString();
	}
}
