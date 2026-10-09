using System.Collections;
using System.Xml;

namespace System.Security.Cryptography.Xml;

internal class NamespaceFrame
{
	private Hashtable _rendered = new Hashtable();

	private Hashtable _unrendered = new Hashtable();

	internal NamespaceFrame()
	{
	}

	internal void AddRendered(XmlAttribute attr)
	{
		_rendered.Add(Utils.GetNamespacePrefix(attr), attr);
	}

	internal XmlAttribute GetRendered(string nsPrefix)
	{
		return (XmlAttribute)_rendered[nsPrefix];
	}

	internal void AddUnrendered(XmlAttribute attr)
	{
		_unrendered.Add(Utils.GetNamespacePrefix(attr), attr);
	}

	internal XmlAttribute GetUnrendered(string nsPrefix)
	{
		return (XmlAttribute)_unrendered[nsPrefix];
	}

	internal Hashtable GetUnrendered()
	{
		return _unrendered;
	}
}
