using System.Collections.Generic;
using System.Reflection;

namespace System.Xml.Xsl.Runtime;

internal class XmlExtensionFunctionTable
{
	private Dictionary<XmlExtensionFunction, XmlExtensionFunction> table;

	private XmlExtensionFunction funcCached;

	public XmlExtensionFunctionTable()
	{
		table = new Dictionary<XmlExtensionFunction, XmlExtensionFunction>();
	}

	public XmlExtensionFunction Bind(string name, string namespaceUri, int numArgs, Type objectType, BindingFlags flags)
	{
		if (funcCached == null)
		{
			funcCached = new XmlExtensionFunction();
		}
		funcCached.Init(name, namespaceUri, numArgs, objectType, flags);
		if (!table.TryGetValue(funcCached, out var value))
		{
			value = funcCached;
			funcCached = null;
			value.Bind();
			table.Add(value, value);
		}
		return value;
	}
}
