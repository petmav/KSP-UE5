using System.Configuration;
using Unity;

namespace System.Xml.Serialization.Configuration;

public sealed class DateTimeSerializationSection : ConfigurationSection
{
	public enum DateTimeSerializationMode
	{
		Default = 0,
		Local = 2,
		Roundtrip = 1
	}

	public DateTimeSerializationMode Mode
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(DateTimeSerializationMode);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public DateTimeSerializationSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
