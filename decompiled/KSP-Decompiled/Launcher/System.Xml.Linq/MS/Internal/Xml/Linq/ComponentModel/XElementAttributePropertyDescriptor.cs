using System;
using System.Xml.Linq;

namespace MS.Internal.Xml.Linq.ComponentModel;

internal class XElementAttributePropertyDescriptor : XPropertyDescriptor<XElement, object>
{
	private XDeferredSingleton<XAttribute> value;

	private XAttribute changeState;

	public XElementAttributePropertyDescriptor()
		: base("Attribute")
	{
	}

	public override object GetValue(object component)
	{
		return value = new XDeferredSingleton<XAttribute>((XElement e, XName n) => e.Attribute(n), component as XElement, null);
	}

	protected override void OnChanged(object sender, XObjectChangeEventArgs args)
	{
		if (value == null)
		{
			return;
		}
		switch (args.ObjectChange)
		{
		case XObjectChange.Add:
			if (sender is XAttribute xAttribute2 && value.element == xAttribute2.parent && value.name == xAttribute2.Name)
			{
				OnValueChanged(value.element, EventArgs.Empty);
			}
			break;
		case XObjectChange.Remove:
			if (sender is XAttribute xAttribute && changeState == xAttribute)
			{
				changeState = null;
				OnValueChanged(value.element, EventArgs.Empty);
			}
			break;
		}
	}

	protected override void OnChanging(object sender, XObjectChangeEventArgs args)
	{
		if (value != null)
		{
			XObjectChange objectChange = args.ObjectChange;
			if (objectChange == XObjectChange.Remove)
			{
				XAttribute xAttribute = sender as XAttribute;
				changeState = ((xAttribute != null && value.element == xAttribute.parent && value.name == xAttribute.Name) ? xAttribute : null);
			}
		}
	}
}
