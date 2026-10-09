using System.Collections;
using System.IO;
using System.Security.Permissions;
using System.Xml;

namespace System.Configuration;

internal class ConfigurationData
{
	private ConfigurationData parent;

	private Hashtable factories;

	private static object removedMark = new object();

	private static object emptyMark = new object();

	private Hashtable pending;

	private string fileName;

	private static object groupMark = new object();

	private Hashtable cache;

	private Hashtable FileCache
	{
		get
		{
			if (cache != null)
			{
				return cache;
			}
			cache = new Hashtable();
			return cache;
		}
	}

	public ConfigurationData()
		: this(null)
	{
	}

	public ConfigurationData(ConfigurationData parent)
	{
		this.parent = ((parent == this) ? null : parent);
		factories = new Hashtable();
	}

	[FileIOPermission(SecurityAction.Assert, Unrestricted = true)]
	public bool Load(string fileName)
	{
		this.fileName = fileName;
		if (fileName == null || !File.Exists(fileName))
		{
			return false;
		}
		XmlTextReader xmlTextReader = null;
		try
		{
			xmlTextReader = new XmlTextReader(new FileStream(fileName, FileMode.Open, FileAccess.Read));
			if (InitRead(xmlTextReader))
			{
				ReadConfigFile(xmlTextReader);
			}
		}
		catch (ConfigurationException)
		{
			throw;
		}
		catch (Exception inner)
		{
			throw new ConfigurationException("Error reading " + fileName, inner);
		}
		finally
		{
			xmlTextReader?.Close();
		}
		return true;
	}

	public bool LoadString(string data)
	{
		if (data == null)
		{
			return false;
		}
		XmlTextReader xmlTextReader = null;
		try
		{
			xmlTextReader = new XmlTextReader(new StringReader(data));
			if (InitRead(xmlTextReader))
			{
				ReadConfigFile(xmlTextReader);
			}
		}
		catch (ConfigurationException)
		{
			throw;
		}
		catch (Exception inner)
		{
			throw new ConfigurationException("Error reading " + fileName, inner);
		}
		finally
		{
			xmlTextReader?.Close();
		}
		return true;
	}

	private object GetHandler(string sectionName)
	{
		lock (factories)
		{
			object obj = factories[sectionName];
			if (obj == null || obj == removedMark)
			{
				if (parent != null)
				{
					return parent.GetHandler(sectionName);
				}
				return null;
			}
			if (obj is IConfigurationSectionHandler)
			{
				return (IConfigurationSectionHandler)obj;
			}
			obj = CreateNewHandler(sectionName, (SectionData)obj);
			factories[sectionName] = obj;
			return obj;
		}
	}

	private object CreateNewHandler(string sectionName, SectionData section)
	{
		Type type = Type.GetType(section.TypeName);
		if (type == null)
		{
			throw new ConfigurationException("Cannot get Type for " + section.TypeName);
		}
		return Activator.CreateInstance(type, nonPublic: true) ?? throw new ConfigurationException("Cannot get instance for " + type);
	}

	private XmlDocument GetInnerDoc(XmlDocument doc, int i, string[] sectionPath)
	{
		if (++i >= sectionPath.Length)
		{
			return doc;
		}
		if (doc.DocumentElement == null)
		{
			return null;
		}
		for (XmlNode xmlNode = doc.DocumentElement.FirstChild; xmlNode != null; xmlNode = xmlNode.NextSibling)
		{
			if (xmlNode.Name == sectionPath[i])
			{
				ConfigXmlDocument configXmlDocument = new ConfigXmlDocument();
				configXmlDocument.Load(new StringReader(xmlNode.OuterXml));
				return GetInnerDoc(configXmlDocument, i, sectionPath);
			}
		}
		return null;
	}

	private XmlDocument GetDocumentForSection(string sectionName)
	{
		ConfigXmlDocument configXmlDocument = new ConfigXmlDocument();
		if (pending == null)
		{
			return configXmlDocument;
		}
		string[] array = sectionName.Split('/');
		if (!(pending[array[0]] is string s))
		{
			return configXmlDocument;
		}
		XmlTextReader xmlTextReader = new XmlTextReader(new StringReader(s));
		xmlTextReader.MoveToContent();
		configXmlDocument.LoadSingleElement(fileName, xmlTextReader);
		return GetInnerDoc(configXmlDocument, 0, array);
	}

	private object GetConfigInternal(string sectionName)
	{
		object handler = GetHandler(sectionName);
		if (!(handler is IConfigurationSectionHandler configurationSectionHandler))
		{
			return handler;
		}
		object result = null;
		if (parent != null)
		{
			result = parent.GetConfig(sectionName);
		}
		XmlDocument documentForSection = GetDocumentForSection(sectionName);
		if (documentForSection == null || documentForSection.DocumentElement == null)
		{
			return result;
		}
		return configurationSectionHandler.Create(result, fileName, documentForSection.DocumentElement);
	}

	public object GetConfig(string sectionName)
	{
		object obj;
		lock (this)
		{
			obj = FileCache[sectionName];
		}
		if (obj == emptyMark)
		{
			return null;
		}
		if (obj != null)
		{
			return obj;
		}
		lock (this)
		{
			obj = GetConfigInternal(sectionName);
			FileCache[sectionName] = ((obj == null) ? emptyMark : obj);
			return obj;
		}
	}

	private object LookForFactory(string key)
	{
		object obj = factories[key];
		if (obj != null)
		{
			return obj;
		}
		if (parent != null)
		{
			return parent.LookForFactory(key);
		}
		return null;
	}

	private bool InitRead(XmlTextReader reader)
	{
		reader.MoveToContent();
		if (reader.NodeType != XmlNodeType.Element || reader.Name != "configuration")
		{
			ThrowException("Configuration file does not have a valid root element", reader);
		}
		if (reader.HasAttributes)
		{
			ThrowException("Unrecognized attribute in root element", reader);
		}
		if (reader.IsEmptyElement)
		{
			reader.Skip();
			return false;
		}
		reader.Read();
		reader.MoveToContent();
		return reader.NodeType != XmlNodeType.EndElement;
	}

	private void MoveToNextElement(XmlTextReader reader)
	{
		while (reader.Read())
		{
			switch (reader.NodeType)
			{
			case XmlNodeType.Element:
				return;
			case XmlNodeType.Comment:
			case XmlNodeType.Whitespace:
			case XmlNodeType.SignificantWhitespace:
			case XmlNodeType.EndElement:
				continue;
			}
			ThrowException("Unrecognized element", reader);
		}
	}

	private void ReadSection(XmlTextReader reader, string sectionName)
	{
		string text = null;
		string text2 = null;
		string text3 = null;
		string text4 = null;
		bool flag = false;
		string text5 = null;
		bool flag2 = true;
		AllowDefinition allowDefinition = AllowDefinition.Everywhere;
		while (reader.MoveToNextAttribute())
		{
			switch (reader.Name)
			{
			case "allowLocation":
				if (text3 != null)
				{
					ThrowException("Duplicated allowLocation attribute.", reader);
				}
				text3 = reader.Value;
				flag2 = text3 == "true";
				if (!flag2 && text3 != "false")
				{
					ThrowException("Invalid attribute value", reader);
				}
				break;
			case "requirePermission":
				if (text5 != null)
				{
					ThrowException("Duplicated requirePermission attribute.", reader);
				}
				text5 = reader.Value;
				flag = text5 == "true";
				if (!flag && text5 != "false")
				{
					ThrowException("Invalid attribute value", reader);
				}
				break;
			case "allowDefinition":
				if (text4 != null)
				{
					ThrowException("Duplicated allowDefinition attribute.", reader);
				}
				text4 = reader.Value;
				try
				{
					allowDefinition = (AllowDefinition)Enum.Parse(typeof(AllowDefinition), text4);
				}
				catch
				{
					ThrowException("Invalid attribute value", reader);
				}
				break;
			case "type":
				if (text2 != null)
				{
					ThrowException("Duplicated type attribute.", reader);
				}
				text2 = reader.Value;
				break;
			case "name":
				if (text != null)
				{
					ThrowException("Duplicated name attribute.", reader);
				}
				text = reader.Value;
				if (text == "location")
				{
					ThrowException("location is a reserved section name", reader);
				}
				break;
			default:
				ThrowException("Unrecognized attribute.", reader);
				break;
			case null:
				break;
			}
		}
		if (text == null || text2 == null)
		{
			ThrowException("Required attribute missing", reader);
		}
		if (sectionName != null)
		{
			text = sectionName + "/" + text;
		}
		reader.MoveToElement();
		object obj2 = LookForFactory(text);
		if (obj2 != null && obj2 != removedMark)
		{
			ThrowException("Already have a factory for " + text, reader);
		}
		SectionData sectionData = new SectionData(text, text2, flag2, allowDefinition, flag);
		sectionData.FileName = fileName;
		factories[text] = sectionData;
		if (reader.IsEmptyElement)
		{
			reader.Skip();
		}
		else
		{
			reader.Read();
			reader.MoveToContent();
			if (reader.NodeType != XmlNodeType.EndElement)
			{
				ReadSections(reader, text);
			}
			reader.ReadEndElement();
		}
		reader.MoveToContent();
	}

	private void ReadRemoveSection(XmlTextReader reader, string sectionName)
	{
		if (!reader.MoveToNextAttribute() || reader.Name != "name")
		{
			ThrowException("Unrecognized attribute.", reader);
		}
		string text = reader.Value;
		if (text == null || text.Length == 0)
		{
			ThrowException("Empty name to remove", reader);
		}
		reader.MoveToElement();
		if (sectionName != null)
		{
			text = sectionName + "/" + text;
		}
		object obj = LookForFactory(text);
		if (obj != null && obj == removedMark)
		{
			ThrowException("No factory for " + text, reader);
		}
		factories[text] = removedMark;
		MoveToNextElement(reader);
	}

	private void ReadSectionGroup(XmlTextReader reader, string configSection)
	{
		if (!reader.MoveToNextAttribute())
		{
			ThrowException("sectionGroup must have a 'name' attribute.", reader);
		}
		string text = null;
		do
		{
			if (reader.Name == "name")
			{
				if (text != null)
				{
					ThrowException("Duplicate 'name' attribute.", reader);
				}
				text = reader.Value;
			}
			else if (reader.Name != "type")
			{
				ThrowException("Unrecognized attribute.", reader);
			}
		}
		while (reader.MoveToNextAttribute());
		if (text == null)
		{
			ThrowException("No 'name' attribute.", reader);
		}
		if (text == "location")
		{
			ThrowException("location is a reserved section name", reader);
		}
		if (configSection != null)
		{
			text = configSection + "/" + text;
		}
		object obj = LookForFactory(text);
		if (obj != null && obj != removedMark && obj != groupMark)
		{
			ThrowException("Already have a factory for " + text, reader);
		}
		factories[text] = groupMark;
		if (reader.IsEmptyElement)
		{
			reader.Skip();
			reader.MoveToContent();
			return;
		}
		reader.Read();
		reader.MoveToContent();
		if (reader.NodeType != XmlNodeType.EndElement)
		{
			ReadSections(reader, text);
		}
		reader.ReadEndElement();
		reader.MoveToContent();
	}

	private void ReadSections(XmlTextReader reader, string configSection)
	{
		int depth = reader.Depth;
		reader.MoveToContent();
		while (reader.Depth == depth)
		{
			switch (reader.Name)
			{
			case "section":
				ReadSection(reader, configSection);
				break;
			case "remove":
				ReadRemoveSection(reader, configSection);
				break;
			case "clear":
				if (reader.HasAttributes)
				{
					ThrowException("Unrecognized attribute.", reader);
				}
				factories.Clear();
				MoveToNextElement(reader);
				break;
			case "sectionGroup":
				ReadSectionGroup(reader, configSection);
				break;
			default:
				ThrowException("Unrecognized element: " + reader.Name, reader);
				break;
			}
			reader.MoveToContent();
		}
	}

	private void StorePending(string name, XmlTextReader reader)
	{
		if (pending == null)
		{
			pending = new Hashtable();
		}
		pending[name] = reader.ReadOuterXml();
	}

	private void ReadConfigFile(XmlTextReader reader)
	{
		reader.MoveToContent();
		while (!reader.EOF && reader.NodeType != XmlNodeType.EndElement)
		{
			string name = reader.Name;
			if (name == "configSections")
			{
				if (reader.HasAttributes)
				{
					ThrowException("Unrecognized attribute in <configSections>.", reader);
				}
				if (reader.IsEmptyElement)
				{
					reader.Skip();
				}
				else
				{
					reader.Read();
					reader.MoveToContent();
					if (reader.NodeType != XmlNodeType.EndElement)
					{
						ReadSections(reader, null);
					}
					reader.ReadEndElement();
				}
			}
			else if (name != null && name != "")
			{
				StorePending(name, reader);
				MoveToNextElement(reader);
			}
			else
			{
				MoveToNextElement(reader);
			}
			reader.MoveToContent();
		}
	}

	private void ThrowException(string text, XmlTextReader reader)
	{
		throw new ConfigurationException(text, fileName, reader.LineNumber);
	}
}
