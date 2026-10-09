namespace System.ComponentModel;

[AttributeUsage(AttributeTargets.All)]
public sealed class ExtenderProvidedPropertyAttribute : Attribute
{
	private PropertyDescriptor extenderProperty;

	private IExtenderProvider provider;

	private Type receiverType;

	public PropertyDescriptor ExtenderProperty => extenderProperty;

	public IExtenderProvider Provider => provider;

	public Type ReceiverType => receiverType;

	internal static ExtenderProvidedPropertyAttribute Create(PropertyDescriptor extenderProperty, Type receiverType, IExtenderProvider provider)
	{
		return new ExtenderProvidedPropertyAttribute
		{
			extenderProperty = extenderProperty,
			receiverType = receiverType,
			provider = provider
		};
	}

	public override bool Equals(object obj)
	{
		if (obj == this)
		{
			return true;
		}
		if (obj is ExtenderProvidedPropertyAttribute extenderProvidedPropertyAttribute && extenderProvidedPropertyAttribute.extenderProperty.Equals(extenderProperty) && extenderProvidedPropertyAttribute.provider.Equals(provider))
		{
			return extenderProvidedPropertyAttribute.receiverType.Equals(receiverType);
		}
		return false;
	}

	public override int GetHashCode()
	{
		return base.GetHashCode();
	}

	public override bool IsDefaultAttribute()
	{
		return receiverType == null;
	}
}
