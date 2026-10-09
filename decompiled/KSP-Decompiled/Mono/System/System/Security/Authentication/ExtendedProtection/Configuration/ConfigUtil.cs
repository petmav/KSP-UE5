using System.ComponentModel;
using System.Configuration;
using System.Reflection;

namespace System.Security.Authentication.ExtendedProtection.Configuration;

internal static class ConfigUtil
{
	internal static T GetCustomAttribute<T>(MemberInfo m, bool inherit)
	{
		object[] customAttributes = m.GetCustomAttributes(typeof(T), inherit: false);
		if (customAttributes.Length == 0)
		{
			return default(T);
		}
		return (T)customAttributes[0];
	}

	internal static ConfigurationProperty BuildProperty(Type t, string name)
	{
		PropertyInfo property = t.GetProperty(name);
		ConfigurationPropertyAttribute customAttribute = GetCustomAttribute<ConfigurationPropertyAttribute>(property, inherit: false);
		TypeConverterAttribute customAttribute2 = GetCustomAttribute<TypeConverterAttribute>(property, inherit: false);
		ConfigurationValidatorAttribute customAttribute3 = GetCustomAttribute<ConfigurationValidatorAttribute>(property, inherit: false);
		return new ConfigurationProperty(customAttribute.Name, property.PropertyType, customAttribute.DefaultValue, (customAttribute2 != null) ? ((TypeConverter)Activator.CreateInstance(Type.GetType(customAttribute2.ConverterTypeName))) : null, customAttribute3?.ValidatorInstance, customAttribute.Options);
	}
}
