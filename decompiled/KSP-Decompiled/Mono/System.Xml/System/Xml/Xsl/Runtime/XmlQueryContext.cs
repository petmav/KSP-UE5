using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Reflection;
using System.Xml.Schema;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public sealed class XmlQueryContext
{
	private XmlQueryRuntime runtime;

	private XPathNavigator defaultDataSource;

	private XmlResolver dataSources;

	private Hashtable dataSourceCache;

	private XsltArgumentList argList;

	private XmlExtensionFunctionTable extFuncsLate;

	private WhitespaceRuleLookup wsRules;

	private QueryReaderSettings readerSettings;

	public XmlNameTable QueryNameTable => readerSettings.NameTable;

	public XmlNameTable DefaultNameTable
	{
		get
		{
			if (defaultDataSource == null)
			{
				return null;
			}
			return defaultDataSource.NameTable;
		}
	}

	public XPathNavigator DefaultDataSource
	{
		get
		{
			if (defaultDataSource == null)
			{
				throw new XslTransformException("Query requires a default data source, but no default was supplied to the query engine.", string.Empty);
			}
			return defaultDataSource;
		}
	}

	internal XmlQueryContext(XmlQueryRuntime runtime, object defaultDataSource, XmlResolver dataSources, XsltArgumentList argList, WhitespaceRuleLookup wsRules)
	{
		this.runtime = runtime;
		this.dataSources = dataSources;
		dataSourceCache = new Hashtable();
		this.argList = argList;
		this.wsRules = wsRules;
		if (defaultDataSource is XmlReader)
		{
			readerSettings = new QueryReaderSettings((XmlReader)defaultDataSource);
		}
		else
		{
			readerSettings = new QueryReaderSettings(new NameTable());
		}
		if (defaultDataSource is string)
		{
			this.defaultDataSource = GetDataSource(defaultDataSource as string, null);
			if (this.defaultDataSource == null)
			{
				throw new XslTransformException("Data source '{0}' cannot be located.", defaultDataSource as string);
			}
		}
		else if (defaultDataSource != null)
		{
			this.defaultDataSource = ConstructDocument(defaultDataSource, null, null);
		}
	}

	public XPathNavigator GetDataSource(string uriRelative, string uriBase)
	{
		XPathNavigator xPathNavigator = null;
		try
		{
			Uri baseUri = ((uriBase != null) ? dataSources.ResolveUri(null, uriBase) : null);
			Uri uri = dataSources.ResolveUri(baseUri, uriRelative);
			if (uri != null)
			{
				xPathNavigator = dataSourceCache[uri] as XPathNavigator;
			}
			if (xPathNavigator == null)
			{
				object entity = dataSources.GetEntity(uri, null, null);
				if (entity != null)
				{
					xPathNavigator = ConstructDocument(entity, uriRelative, uri);
					dataSourceCache.Add(uri, xPathNavigator);
				}
			}
		}
		catch (XslTransformException)
		{
			throw;
		}
		catch (Exception ex2)
		{
			if (!XmlException.IsCatchableException(ex2))
			{
				throw;
			}
			throw new XslTransformException(ex2, "An error occurred while loading document '{0}'. See InnerException for a complete description of the error.", uriRelative);
		}
		return xPathNavigator;
	}

	private XPathNavigator ConstructDocument(object dataSource, string uriRelative, Uri uriResolved)
	{
		if (dataSource is Stream stream)
		{
			XmlReader xmlReader = readerSettings.CreateReader(stream, (uriResolved != null) ? uriResolved.ToString() : null);
			try
			{
				return new XPathDocument(WhitespaceRuleReader.CreateReader(xmlReader, wsRules), XmlSpace.Preserve).CreateNavigator();
			}
			finally
			{
				xmlReader.Close();
			}
		}
		if (dataSource is XmlReader)
		{
			return new XPathDocument(WhitespaceRuleReader.CreateReader(dataSource as XmlReader, wsRules), XmlSpace.Preserve).CreateNavigator();
		}
		if (dataSource is IXPathNavigable)
		{
			if (wsRules != null)
			{
				throw new XslTransformException("White space cannot be stripped from input documents that have already been loaded. Provide the input document as an XmlReader instead.", string.Empty);
			}
			return (dataSource as IXPathNavigable).CreateNavigator();
		}
		throw new XslTransformException("Cannot query the data source object referenced by URI '{0}', because the provided XmlResolver returned an object of type '{1}'. Only Stream, XmlReader, and IXPathNavigable data source objects are currently supported.", uriRelative, dataSource.GetType().ToString());
	}

	public object GetParameter(string localName, string namespaceUri)
	{
		if (argList == null)
		{
			return null;
		}
		return argList.GetParam(localName, namespaceUri);
	}

	public object GetLateBoundObject(string namespaceUri)
	{
		if (argList == null)
		{
			return null;
		}
		return argList.GetExtensionObject(namespaceUri);
	}

	public bool LateBoundFunctionExists(string name, string namespaceUri)
	{
		if (argList == null)
		{
			return false;
		}
		object extensionObject = argList.GetExtensionObject(namespaceUri);
		if (extensionObject == null)
		{
			return false;
		}
		return new XmlExtensionFunction(name, namespaceUri, -1, extensionObject.GetType(), BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public).CanBind();
	}

	public IList<XPathItem> InvokeXsltLateBoundFunction(string name, string namespaceUri, IList<XPathItem>[] args)
	{
		object obj = ((argList != null) ? argList.GetExtensionObject(namespaceUri) : null);
		if (obj == null)
		{
			throw new XslTransformException("Cannot find a script or an extension object associated with namespace '{0}'.", namespaceUri);
		}
		if (extFuncsLate == null)
		{
			extFuncsLate = new XmlExtensionFunctionTable();
		}
		XmlExtensionFunction xmlExtensionFunction = extFuncsLate.Bind(name, namespaceUri, args.Length, obj.GetType(), BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
		object[] array = new object[args.Length];
		for (int i = 0; i < args.Length; i++)
		{
			XmlQueryType xmlArgumentType = xmlExtensionFunction.GetXmlArgumentType(i);
			switch (xmlArgumentType.TypeCode)
			{
			case XmlTypeCode.Boolean:
				array[i] = XsltConvert.ToBoolean(args[i]);
				break;
			case XmlTypeCode.Double:
				array[i] = XsltConvert.ToDouble(args[i]);
				break;
			case XmlTypeCode.String:
				array[i] = XsltConvert.ToString(args[i]);
				break;
			case XmlTypeCode.Node:
				if (xmlArgumentType.IsSingleton)
				{
					array[i] = XsltConvert.ToNode(args[i]);
				}
				else
				{
					array[i] = XsltConvert.ToNodeSet(args[i]);
				}
				break;
			case XmlTypeCode.Item:
				array[i] = args[i];
				break;
			}
			Type clrArgumentType = xmlExtensionFunction.GetClrArgumentType(i);
			if (xmlArgumentType.TypeCode == XmlTypeCode.Item || !clrArgumentType.IsAssignableFrom(array[i].GetType()))
			{
				array[i] = runtime.ChangeTypeXsltArgument(xmlArgumentType, array[i], clrArgumentType);
			}
		}
		object obj2 = xmlExtensionFunction.Invoke(obj, array);
		if (obj2 == null && xmlExtensionFunction.ClrReturnType == XsltConvert.VoidType)
		{
			return XmlQueryNodeSequence.Empty;
		}
		return (IList<XPathItem>)runtime.ChangeTypeXsltResult(XmlQueryTypeFactory.ItemS, obj2);
	}

	public void OnXsltMessageEncountered(string message)
	{
		XsltMessageEncounteredEventHandler xsltMessageEncounteredEventHandler = ((argList != null) ? argList.xsltMessageEncountered : null);
		if (xsltMessageEncounteredEventHandler != null)
		{
			xsltMessageEncounteredEventHandler(this, new XmlILQueryEventArgs(message));
		}
		else
		{
			Console.WriteLine(message);
		}
	}
}
