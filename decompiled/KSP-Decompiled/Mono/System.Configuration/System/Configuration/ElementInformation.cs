using System.Collections;
using Unity;

namespace System.Configuration;

public sealed class ElementInformation
{
	private readonly PropertyInformation propertyInfo;

	private readonly ConfigurationElement owner;

	private readonly PropertyInformationCollection properties;

	[System.MonoTODO]
	public ICollection Errors
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public bool IsCollection => owner is ConfigurationElementCollection;

	public bool IsLocked
	{
		get
		{
			if (propertyInfo == null)
			{
				return false;
			}
			return propertyInfo.IsLocked;
		}
	}

	[System.MonoTODO("Support multiple levels of inheritance")]
	public bool IsPresent => owner.IsElementPresent;

	public int LineNumber
	{
		get
		{
			if (propertyInfo == null)
			{
				return 0;
			}
			return propertyInfo.LineNumber;
		}
	}

	public string Source
	{
		get
		{
			if (propertyInfo == null)
			{
				return null;
			}
			return propertyInfo.Source;
		}
	}

	public Type Type
	{
		get
		{
			if (propertyInfo == null)
			{
				return owner.GetType();
			}
			return propertyInfo.Type;
		}
	}

	public ConfigurationValidatorBase Validator
	{
		get
		{
			if (propertyInfo == null)
			{
				return new DefaultValidator();
			}
			return propertyInfo.Validator;
		}
	}

	public PropertyInformationCollection Properties => properties;

	internal ElementInformation(ConfigurationElement owner, PropertyInformation propertyInfo)
	{
		this.propertyInfo = propertyInfo;
		this.owner = owner;
		properties = new PropertyInformationCollection();
		foreach (ConfigurationProperty property in owner.Properties)
		{
			properties.Add(new PropertyInformation(owner, property));
		}
	}

	internal void Reset(ElementInformation parentInfo)
	{
		foreach (PropertyInformation property in Properties)
		{
			PropertyInformation parentProperty = parentInfo.Properties[property.Name];
			property.Reset(parentProperty);
		}
	}

	internal ElementInformation()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
