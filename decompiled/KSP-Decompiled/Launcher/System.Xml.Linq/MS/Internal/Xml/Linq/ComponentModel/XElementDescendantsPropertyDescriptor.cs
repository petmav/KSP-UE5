using System;
using System.Collections.Generic;
using System.Xml.Linq;

namespace MS.Internal.Xml.Linq.ComponentModel;

internal class XElementDescendantsPropertyDescriptor : XPropertyDescriptor<XElement, IEnumerable<XElement>>
{
	private XDeferredAxis<XElement> value;

	private XName changeState;

	public XElementDescendantsPropertyDescriptor()
		: base("Descendants")
	{
	}

	public override object GetValue(object component)
	{
		return value = new XDeferredAxis<XElement>((XElement e, XName n) => (!(n != null)) ? e.Descendants() : e.Descendants(n), component as XElement, null);
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
		case XObjectChange.Remove:
			if (sender is XElement xElement2 && (value.name == xElement2.Name || value.name == null))
			{
				OnValueChanged(value.element, EventArgs.Empty);
			}
			break;
		case XObjectChange.Name:
			if (sender is XElement xElement && value.element != xElement && value.name != null && (value.name == xElement.Name || value.name == changeState))
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
			if (objectChange == XObjectChange.Name)
			{
				changeState = (sender as XElement)?.Name;
			}
		}
	}
}
