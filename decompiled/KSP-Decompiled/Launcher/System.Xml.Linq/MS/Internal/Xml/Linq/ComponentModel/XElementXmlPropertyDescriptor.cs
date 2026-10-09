using System;
using System.Xml.Linq;

namespace MS.Internal.Xml.Linq.ComponentModel;

internal class XElementXmlPropertyDescriptor : XPropertyDescriptor<XElement, string>
{
	private XElement element;

	public XElementXmlPropertyDescriptor()
		: base("Xml")
	{
	}

	public override object GetValue(object component)
	{
		element = component as XElement;
		if (element == null)
		{
			return string.Empty;
		}
		return element.ToString(SaveOptions.DisableFormatting);
	}

	protected override void OnChanged(object sender, XObjectChangeEventArgs args)
	{
		if (element != null)
		{
			OnValueChanged(element, EventArgs.Empty);
		}
	}
}
