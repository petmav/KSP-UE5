using System.ComponentModel;
using Unity;

namespace System.Configuration;

public sealed class ConfigurationProperty
{
	public TypeConverter Converter
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public object DefaultValue
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string Description
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool IsAssemblyStringTransformationRequired
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool IsDefaultCollection
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool IsKey
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool IsRequired
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool IsTypeStringTransformationRequired
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool IsVersionCheckRequired
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public string Name
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public Type Type
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConfigurationValidatorBase Validator
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConfigurationProperty(string name, Type type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationProperty(string name, Type type, object defaultValue)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationProperty(string name, Type type, object defaultValue, TypeConverter typeConverter, ConfigurationValidatorBase validator, ConfigurationPropertyOptions options)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationProperty(string name, Type type, object defaultValue, TypeConverter typeConverter, ConfigurationValidatorBase validator, ConfigurationPropertyOptions options, string description)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationProperty(string name, Type type, object defaultValue, ConfigurationPropertyOptions options)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
