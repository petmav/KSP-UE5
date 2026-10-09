using System;
using System.Collections.Generic;
using System.Xml.Linq;

namespace MS.Internal.Xml.Linq.ComponentModel;

internal class XElementElementsPropertyDescriptor : XPropertyDescriptor<XElement, IEnumerable<XElement>>
{
	private XDeferredAxis<XElement> value;

	private object changeState;

	public XElementElementsPropertyDescriptor()
		: base("Elements")
	{
	}

	public override object GetValue(object component)
	{
		return value = new XDeferredAxis<XElement>((XElement e, XName n) => (!(n != null)) ? e.Elements() : e.Elements(n), component as XElement, null);
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
			if (sender is XElement xElement3 && value.element == xElement3.parent && (value.name == xElement3.Name || value.name == null))
			{
				OnValueChanged(value.element, EventArgs.Empty);
			}
			break;
		case XObjectChange.Remove:
			if (sender is XElement xElement2 && value.element == changeState as XContainer && (value.name == xElement2.Name || value.name == null))
			{
				changeState = null;
				OnValueChanged(value.element, EventArgs.Empty);
			}
			break;
		case XObjectChange.Name:
			if (sender is XElement xElement && value.element == xElement.parent && value.name != null && (value.name == xElement.Name || value.name == changeState as XName))
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
			switch (args.ObjectChange)
			{
			case XObjectChange.Remove:
				changeState = (sender as XElement)?.parent;
				break;
			case XObjectChange.Name:
				changeState = (sender as XElement)?.Name;
				break;
			}
		}
	}
}
