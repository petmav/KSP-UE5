using System;
using System.Xml.Linq;

namespace MS.Internal.Xml.Linq.ComponentModel;

internal class XElementValuePropertyDescriptor : XPropertyDescriptor<XElement, string>
{
	private XElement element;

	public override bool IsReadOnly => false;

	public XElementValuePropertyDescriptor()
		: base("Value")
	{
	}

	public override object GetValue(object component)
	{
		element = component as XElement;
		if (element == null)
		{
			return string.Empty;
		}
		return element.Value;
	}

	public override void SetValue(object component, object value)
	{
		element = component as XElement;
		if (element != null)
		{
			element.Value = value as string;
		}
	}

	protected override void OnChanged(object sender, XObjectChangeEventArgs args)
	{
		if (element == null)
		{
			return;
		}
		switch (args.ObjectChange)
		{
		case XObjectChange.Add:
		case XObjectChange.Remove:
			if (sender is XElement || sender is XText)
			{
				OnValueChanged(element, EventArgs.Empty);
			}
			break;
		case XObjectChange.Value:
			if (sender is XText)
			{
				OnValueChanged(element, EventArgs.Empty);
			}
			break;
		}
	}
}
