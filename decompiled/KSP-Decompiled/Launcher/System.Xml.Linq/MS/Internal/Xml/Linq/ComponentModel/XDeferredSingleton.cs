using System;
using System.Xml.Linq;

namespace MS.Internal.Xml.Linq.ComponentModel;

internal class XDeferredSingleton<T> where T : XObject
{
	private Func<XElement, XName, T> func;

	internal XElement element;

	internal XName name;

	public T this[string expandedName]
	{
		get
		{
			if (expandedName == null)
			{
				throw new ArgumentNullException("expandedName");
			}
			if (name == null)
			{
				name = expandedName;
			}
			else if (name != expandedName)
			{
				return null;
			}
			return func(element, name);
		}
	}

	public XDeferredSingleton(Func<XElement, XName, T> func, XElement element, XName name)
	{
		if (func == null)
		{
			throw new ArgumentNullException("func");
		}
		if (element == null)
		{
			throw new ArgumentNullException("element");
		}
		this.func = func;
		this.element = element;
		this.name = name;
	}
}
