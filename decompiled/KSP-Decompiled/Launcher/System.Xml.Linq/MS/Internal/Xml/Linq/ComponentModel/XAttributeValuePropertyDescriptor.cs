using System;
using System.Xml.Linq;

namespace MS.Internal.Xml.Linq.ComponentModel;

internal class XAttributeValuePropertyDescriptor : XPropertyDescriptor<XAttribute, string>
{
	private XAttribute attribute;

	public override bool IsReadOnly => false;

	public XAttributeValuePropertyDescriptor()
		: base("Value")
	{
	}

	public override object GetValue(object component)
	{
		attribute = component as XAttribute;
		if (attribute == null)
		{
			return string.Empty;
		}
		return attribute.Value;
	}

	public override void SetValue(object component, object value)
	{
		attribute = component as XAttribute;
		if (attribute != null)
		{
			attribute.Value = value as string;
		}
	}

	protected override void OnChanged(object sender, XObjectChangeEventArgs args)
	{
		if (attribute != null && args.ObjectChange == XObjectChange.Value)
		{
			OnValueChanged(attribute, EventArgs.Empty);
		}
	}
}
