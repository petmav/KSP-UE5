using System.ComponentModel;

namespace System.Configuration;

public abstract class ConfigurationConverterBase : TypeConverter
{
	public override bool CanConvertFrom(ITypeDescriptorContext ctx, Type type)
	{
		if (type == typeof(string))
		{
			return true;
		}
		return base.CanConvertFrom(ctx, type);
	}

	public override bool CanConvertTo(ITypeDescriptorContext ctx, Type type)
	{
		if (type == typeof(string))
		{
			return true;
		}
		return base.CanConvertTo(ctx, type);
	}
}
