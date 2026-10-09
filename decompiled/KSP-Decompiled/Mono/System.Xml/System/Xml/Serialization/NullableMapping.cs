namespace System.Xml.Serialization;

internal class NullableMapping : TypeMapping
{
	private TypeMapping baseMapping;

	internal TypeMapping BaseMapping
	{
		get
		{
			return baseMapping;
		}
		set
		{
			baseMapping = value;
		}
	}

	internal override string DefaultElementName => BaseMapping.DefaultElementName;
}
