using System;
using System.Xml.Linq;

namespace MS.Internal.Xml.Linq.ComponentModel;

internal class XElementElementPropertyDescriptor : XPropertyDescriptor<XElement, object>
{
	private XDeferredSingleton<XElement> value;

	private XElement changeState;

	public XElementElementPropertyDescriptor()
		: base("Element")
	{
	}

	public override object GetValue(object component)
	{
		return value = new XDeferredSingleton<XElement>((XElement e, XName n) => e.Element(n), component as XElement, null);
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
			if (sender is XElement xElement2 && value.element == xElement2.parent && value.name == xElement2.Name && value.element.Element(value.name) == xElement2)
			{
				OnValueChanged(value.element, EventArgs.Empty);
			}
			break;
		case XObjectChange.Remove:
			if (sender is XElement xElement3 && changeState == xElement3)
			{
				changeState = null;
				OnValueChanged(value.element, EventArgs.Empty);
			}
			break;
		case XObjectChange.Name:
			if (sender is XElement xElement)
			{
				if (value.element == xElement.parent && value.name == xElement.Name && value.element.Element(value.name) == xElement)
				{
					OnValueChanged(value.element, EventArgs.Empty);
				}
				else if (changeState == xElement)
				{
					changeState = null;
					OnValueChanged(value.element, EventArgs.Empty);
				}
			}
			break;
		}
	}

	protected override void OnChanging(object sender, XObjectChangeEventArgs args)
	{
		if (value != null)
		{
			XObjectChange objectChange = args.ObjectChange;
			if ((uint)(objectChange - 1) <= 1u)
			{
				XElement xElement = sender as XElement;
				changeState = ((xElement != null && value.element == xElement.parent && value.name == xElement.Name && value.element.Element(value.name) == xElement) ? xElement : null);
			}
		}
	}
}
