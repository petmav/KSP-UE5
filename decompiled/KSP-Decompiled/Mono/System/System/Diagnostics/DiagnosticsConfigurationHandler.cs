using System.Collections;
using System.Collections.Specialized;
using System.Configuration;
using System.Reflection;
using System.Xml;

namespace System.Diagnostics;

[Obsolete("This class is obsoleted")]
public class DiagnosticsConfigurationHandler : IConfigurationSectionHandler
{
	private delegate void ElementHandler(IDictionary d, XmlNode node);

	private TraceImplSettings configValues;

	private IDictionary elementHandlers = new Hashtable();

	public DiagnosticsConfigurationHandler()
	{
		elementHandlers["assert"] = new ElementHandler(AddAssertNode);
		elementHandlers["performanceCounters"] = new ElementHandler(AddPerformanceCountersNode);
		elementHandlers["switches"] = new ElementHandler(AddSwitchesNode);
		elementHandlers["trace"] = new ElementHandler(AddTraceNode);
		elementHandlers["sources"] = new ElementHandler(AddSourcesNode);
	}

	public virtual object Create(object parent, object configContext, XmlNode section)
	{
		IDictionary dictionary = ((parent != null) ? ((IDictionary)((ICloneable)parent).Clone()) : new Hashtable(CaseInsensitiveHashCodeProvider.Default, CaseInsensitiveComparer.Default));
		if (dictionary.Contains(".__TraceInfoSettingsKey__."))
		{
			configValues = (TraceImplSettings)dictionary[".__TraceInfoSettingsKey__."];
		}
		else
		{
			dictionary.Add(".__TraceInfoSettingsKey__.", configValues = new TraceImplSettings());
		}
		foreach (XmlNode childNode in section.ChildNodes)
		{
			XmlNodeType nodeType = childNode.NodeType;
			if (nodeType == XmlNodeType.Element && !(childNode.LocalName != "sharedListeners"))
			{
				AddTraceListeners(dictionary, childNode, GetSharedListeners(dictionary));
			}
		}
		foreach (XmlNode childNode2 in section.ChildNodes)
		{
			switch (childNode2.NodeType)
			{
			case XmlNodeType.Element:
				if (!(childNode2.LocalName == "sharedListeners"))
				{
					ElementHandler elementHandler = (ElementHandler)elementHandlers[childNode2.Name];
					if (elementHandler != null)
					{
						elementHandler(dictionary, childNode2);
					}
					else
					{
						ThrowUnrecognizedElement(childNode2);
					}
				}
				break;
			default:
				ThrowUnrecognizedElement(childNode2);
				break;
			case XmlNodeType.Comment:
			case XmlNodeType.Whitespace:
				break;
			}
		}
		return dictionary;
	}

	private void AddAssertNode(IDictionary d, XmlNode node)
	{
		XmlAttributeCollection attributes = node.Attributes;
		string attribute = GetAttribute(attributes, "assertuienabled", required: false, node);
		string attribute2 = GetAttribute(attributes, "logfilename", required: false, node);
		ValidateInvalidAttributes(attributes, node);
		if (attribute != null)
		{
			try
			{
				d["assertuienabled"] = bool.Parse(attribute);
			}
			catch (Exception inner)
			{
				throw new ConfigurationException("The `assertuienabled' attribute must be `true' or `false'", inner, node);
			}
		}
		if (attribute2 != null)
		{
			d["logfilename"] = attribute2;
		}
		DefaultTraceListener defaultTraceListener = (DefaultTraceListener)configValues.Listeners["Default"];
		if (defaultTraceListener != null)
		{
			if (attribute != null)
			{
				defaultTraceListener.AssertUiEnabled = (bool)d["assertuienabled"];
			}
			if (attribute2 != null)
			{
				defaultTraceListener.LogFileName = attribute2;
			}
		}
		if (node.ChildNodes.Count > 0)
		{
			ThrowUnrecognizedElement(node.ChildNodes[0]);
		}
	}

	private void AddPerformanceCountersNode(IDictionary d, XmlNode node)
	{
		XmlAttributeCollection attributes = node.Attributes;
		string attribute = GetAttribute(attributes, "filemappingsize", required: false, node);
		ValidateInvalidAttributes(attributes, node);
		if (attribute != null)
		{
			try
			{
				d["filemappingsize"] = int.Parse(attribute);
			}
			catch (Exception inner)
			{
				throw new ConfigurationException("The `filemappingsize' attribute must be an integral value.", inner, node);
			}
		}
		if (node.ChildNodes.Count > 0)
		{
			ThrowUnrecognizedElement(node.ChildNodes[0]);
		}
	}

	private void AddSwitchesNode(IDictionary d, XmlNode node)
	{
		ValidateInvalidAttributes(node.Attributes, node);
		IDictionary dictionary = new Hashtable();
		foreach (XmlNode childNode in node.ChildNodes)
		{
			switch (childNode.NodeType)
			{
			case XmlNodeType.Element:
			{
				XmlAttributeCollection attributes = childNode.Attributes;
				string text = null;
				string text2 = null;
				switch (childNode.Name)
				{
				case "add":
					text = GetAttribute(attributes, "name", required: true, childNode);
					text2 = GetAttribute(attributes, "value", required: true, childNode);
					dictionary[text] = GetSwitchValue(text, text2);
					break;
				case "remove":
					text = GetAttribute(attributes, "name", required: true, childNode);
					dictionary.Remove(text);
					break;
				case "clear":
					dictionary.Clear();
					break;
				default:
					ThrowUnrecognizedElement(childNode);
					break;
				}
				ValidateInvalidAttributes(attributes, childNode);
				break;
			}
			default:
				ThrowUnrecognizedNode(childNode);
				break;
			case XmlNodeType.Comment:
			case XmlNodeType.Whitespace:
				break;
			}
		}
		d[node.Name] = dictionary;
	}

	private static object GetSwitchValue(string name, string value)
	{
		return value;
	}

	private void AddTraceNode(IDictionary d, XmlNode node)
	{
		AddTraceAttributes(d, node);
		foreach (XmlNode childNode in node.ChildNodes)
		{
			switch (childNode.NodeType)
			{
			case XmlNodeType.Element:
				if (childNode.Name == "listeners")
				{
					AddTraceListeners(d, childNode, configValues.Listeners);
				}
				else
				{
					ThrowUnrecognizedElement(childNode);
				}
				ValidateInvalidAttributes(childNode.Attributes, childNode);
				break;
			default:
				ThrowUnrecognizedNode(childNode);
				break;
			case XmlNodeType.Comment:
			case XmlNodeType.Whitespace:
				break;
			}
		}
	}

	private void AddTraceAttributes(IDictionary d, XmlNode node)
	{
		XmlAttributeCollection attributes = node.Attributes;
		string attribute = GetAttribute(attributes, "autoflush", required: false, node);
		string attribute2 = GetAttribute(attributes, "indentsize", required: false, node);
		ValidateInvalidAttributes(attributes, node);
		if (attribute != null)
		{
			bool flag = false;
			try
			{
				flag = bool.Parse(attribute);
				d["autoflush"] = flag;
			}
			catch (Exception inner)
			{
				throw new ConfigurationException("The `autoflush' attribute must be `true' or `false'", inner, node);
			}
			configValues.AutoFlush = flag;
		}
		if (attribute2 != null)
		{
			int num = 0;
			try
			{
				num = int.Parse(attribute2);
				d["indentsize"] = num;
			}
			catch (Exception inner2)
			{
				throw new ConfigurationException("The `indentsize' attribute must be an integral value.", inner2, node);
			}
			configValues.IndentSize = num;
		}
	}

	private TraceListenerCollection GetSharedListeners(IDictionary d)
	{
		TraceListenerCollection traceListenerCollection = d["sharedListeners"] as TraceListenerCollection;
		if (traceListenerCollection == null)
		{
			traceListenerCollection = (TraceListenerCollection)(d["sharedListeners"] = new TraceListenerCollection());
		}
		return traceListenerCollection;
	}

	private void AddSourcesNode(IDictionary d, XmlNode node)
	{
		ValidateInvalidAttributes(node.Attributes, node);
		Hashtable hashtable = d["sources"] as Hashtable;
		if (hashtable == null)
		{
			hashtable = (Hashtable)(d["sources"] = new Hashtable());
		}
		foreach (XmlNode childNode in node.ChildNodes)
		{
			switch (childNode.NodeType)
			{
			case XmlNodeType.Element:
				if (childNode.Name == "source")
				{
					AddTraceSource(d, hashtable, childNode);
				}
				else
				{
					ThrowUnrecognizedElement(childNode);
				}
				break;
			default:
				ThrowUnrecognizedNode(childNode);
				break;
			case XmlNodeType.Comment:
			case XmlNodeType.Whitespace:
				break;
			}
		}
	}

	private void AddTraceSource(IDictionary d, Hashtable sources, XmlNode node)
	{
		string text = null;
		SourceLevels levels = SourceLevels.Error;
		StringDictionary stringDictionary = new StringDictionary();
		foreach (XmlAttribute attribute in node.Attributes)
		{
			string name = attribute.Name;
			if (!(name == "name"))
			{
				if (name == "switchValue")
				{
					levels = (SourceLevels)Enum.Parse(typeof(SourceLevels), attribute.Value);
				}
				else
				{
					stringDictionary[attribute.Name] = attribute.Value;
				}
			}
			else
			{
				text = attribute.Value;
			}
		}
		if (text == null)
		{
			throw new ConfigurationException("Mandatory attribute 'name' is missing in 'source' element.");
		}
		if (sources.ContainsKey(text))
		{
			return;
		}
		TraceSourceInfo traceSourceInfo = new TraceSourceInfo(text, levels, configValues);
		sources.Add(traceSourceInfo.Name, traceSourceInfo);
		foreach (XmlNode childNode in node.ChildNodes)
		{
			switch (childNode.NodeType)
			{
			case XmlNodeType.Element:
				if (childNode.Name == "listeners")
				{
					AddTraceListeners(d, childNode, traceSourceInfo.Listeners);
				}
				else
				{
					ThrowUnrecognizedElement(childNode);
				}
				ValidateInvalidAttributes(childNode.Attributes, childNode);
				break;
			default:
				ThrowUnrecognizedNode(childNode);
				break;
			case XmlNodeType.Comment:
			case XmlNodeType.Whitespace:
				break;
			}
		}
	}

	private void AddTraceListeners(IDictionary d, XmlNode listenersNode, TraceListenerCollection listeners)
	{
		ValidateInvalidAttributes(listenersNode.Attributes, listenersNode);
		foreach (XmlNode childNode in listenersNode.ChildNodes)
		{
			switch (childNode.NodeType)
			{
			case XmlNodeType.Element:
			{
				XmlAttributeCollection attributes = childNode.Attributes;
				string text = null;
				switch (childNode.Name)
				{
				case "add":
					AddTraceListener(d, childNode, attributes, listeners);
					break;
				case "remove":
					text = GetAttribute(attributes, "name", required: true, childNode);
					RemoveTraceListener(text);
					break;
				case "clear":
					configValues.Listeners.Clear();
					break;
				default:
					ThrowUnrecognizedElement(childNode);
					break;
				}
				ValidateInvalidAttributes(attributes, childNode);
				break;
			}
			default:
				ThrowUnrecognizedNode(childNode);
				break;
			case XmlNodeType.Comment:
			case XmlNodeType.Whitespace:
				break;
			}
		}
	}

	private void AddTraceListener(IDictionary d, XmlNode child, XmlAttributeCollection attributes, TraceListenerCollection listeners)
	{
		string attribute = GetAttribute(attributes, "name", required: true, child);
		string text = null;
		text = GetAttribute(attributes, "type", required: false, child);
		if (text == null)
		{
			TraceListener traceListener = GetSharedListeners(d)[attribute];
			if (traceListener == null)
			{
				throw new ConfigurationException($"Shared trace listener {attribute} does not exist.");
			}
			if (attributes.Count != 0)
			{
				throw new ConfigurationErrorsException($"Listener '{attribute}' references a shared listener and can only have a 'Name' attribute.");
			}
			traceListener.IndentSize = configValues.IndentSize;
			listeners.Add(traceListener);
			return;
		}
		Type type = Type.GetType(text);
		if (type == null)
		{
			throw new ConfigurationException($"Invalid Type Specified: {text}");
		}
		string attribute2 = GetAttribute(attributes, "initializeData", required: false, child);
		object[] parameters;
		Type[] types;
		if (attribute2 != null)
		{
			parameters = new object[1] { attribute2 };
			types = new Type[1] { typeof(string) };
		}
		else
		{
			parameters = null;
			types = Type.EmptyTypes;
		}
		BindingFlags bindingFlags = BindingFlags.Instance | BindingFlags.Public;
		if (type.Assembly == GetType().Assembly)
		{
			bindingFlags |= BindingFlags.NonPublic;
		}
		ConstructorInfo constructor = type.GetConstructor(bindingFlags, null, types, null);
		if (constructor == null)
		{
			throw new ConfigurationException("Couldn't find constructor for class " + text);
		}
		TraceListener traceListener2 = (TraceListener)constructor.Invoke(parameters);
		traceListener2.Name = attribute;
		string attribute3 = GetAttribute(attributes, "traceOutputOptions", required: false, child);
		if (attribute3 != null)
		{
			if (attribute3 != attribute3.Trim())
			{
				throw new ConfigurationErrorsException($"Invalid value '{attribute3}' for 'traceOutputOptions'.", child);
			}
			TraceOptions traceOutputOptions;
			try
			{
				traceOutputOptions = (TraceOptions)Enum.Parse(typeof(TraceOptions), attribute3);
			}
			catch (ArgumentException)
			{
				throw new ConfigurationErrorsException($"Invalid value '{attribute3}' for 'traceOutputOptions'.", child);
			}
			traceListener2.TraceOutputOptions = traceOutputOptions;
		}
		string[] supportedAttributes = traceListener2.GetSupportedAttributes();
		if (supportedAttributes != null)
		{
			foreach (string text2 in supportedAttributes)
			{
				string attribute4 = GetAttribute(attributes, text2, required: false, child);
				if (attribute4 != null)
				{
					traceListener2.Attributes.Add(text2, attribute4);
				}
			}
		}
		traceListener2.IndentSize = configValues.IndentSize;
		listeners.Add(traceListener2);
	}

	private void RemoveTraceListener(string name)
	{
		try
		{
			configValues.Listeners.Remove(name);
		}
		catch (ArgumentException)
		{
		}
		catch (Exception inner)
		{
			throw new ConfigurationException($"Unknown error removing listener: {name}", inner);
		}
	}

	private string GetAttribute(XmlAttributeCollection attrs, string attr, bool required, XmlNode node)
	{
		XmlAttribute xmlAttribute = attrs[attr];
		string text = null;
		if (xmlAttribute != null)
		{
			text = xmlAttribute.Value;
			if (required)
			{
				ValidateAttribute(attr, text, node);
			}
			attrs.Remove(xmlAttribute);
		}
		else if (required)
		{
			ThrowMissingAttribute(attr, node);
		}
		return text;
	}

	private void ValidateAttribute(string attribute, string value, XmlNode node)
	{
		if (value == null || value.Length == 0)
		{
			throw new ConfigurationException($"Required attribute '{attribute}' cannot be empty.", node);
		}
	}

	private void ValidateInvalidAttributes(XmlAttributeCollection c, XmlNode node)
	{
		if (c.Count != 0)
		{
			ThrowUnrecognizedAttribute(c[0].Name, node);
		}
	}

	private void ThrowMissingAttribute(string attribute, XmlNode node)
	{
		throw new ConfigurationException($"Required attribute '{attribute}' not found.", node);
	}

	private void ThrowUnrecognizedNode(XmlNode node)
	{
		throw new ConfigurationException($"Unrecognized node `{node.Name}'; nodeType={node.NodeType}", node);
	}

	private void ThrowUnrecognizedElement(XmlNode node)
	{
		throw new ConfigurationException($"Unrecognized element '{node.Name}'.", node);
	}

	private void ThrowUnrecognizedAttribute(string attribute, XmlNode node)
	{
		throw new ConfigurationException($"Unrecognized attribute '{attribute}' on element <{node.Name}/>.", node);
	}
}
