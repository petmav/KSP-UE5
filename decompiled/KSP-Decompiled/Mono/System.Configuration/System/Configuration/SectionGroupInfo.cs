using System.Xml;

namespace System.Configuration;

internal class SectionGroupInfo : ConfigInfo
{
	private bool modified;

	private ConfigInfoCollection sections;

	private ConfigInfoCollection groups;

	private static ConfigInfoCollection emptyList = new ConfigInfoCollection();

	public ConfigInfoCollection Sections
	{
		get
		{
			if (sections == null)
			{
				return emptyList;
			}
			return sections;
		}
	}

	public ConfigInfoCollection Groups
	{
		get
		{
			if (groups == null)
			{
				return emptyList;
			}
			return groups;
		}
	}

	public SectionGroupInfo()
	{
		Type = typeof(ConfigurationSectionGroup);
	}

	public SectionGroupInfo(string groupName, string typeName)
	{
		Name = groupName;
		TypeName = typeName;
	}

	public void AddChild(ConfigInfo data)
	{
		modified = true;
		data.Parent = this;
		if (data is SectionInfo)
		{
			if (sections == null)
			{
				sections = new ConfigInfoCollection();
			}
			sections[data.Name] = data;
		}
		else
		{
			if (groups == null)
			{
				groups = new ConfigInfoCollection();
			}
			groups[data.Name] = data;
		}
	}

	public void Clear()
	{
		modified = true;
		if (sections != null)
		{
			sections.Clear();
		}
		if (groups != null)
		{
			groups.Clear();
		}
	}

	public bool HasChild(string name)
	{
		if (sections != null && sections[name] != null)
		{
			return true;
		}
		if (groups != null)
		{
			return groups[name] != null;
		}
		return false;
	}

	public void RemoveChild(string name)
	{
		modified = true;
		if (sections != null)
		{
			sections.Remove(name);
		}
		if (groups != null)
		{
			groups.Remove(name);
		}
	}

	public SectionInfo GetChildSection(string name)
	{
		if (sections != null)
		{
			return sections[name] as SectionInfo;
		}
		return null;
	}

	public SectionGroupInfo GetChildGroup(string name)
	{
		if (groups != null)
		{
			return groups[name] as SectionGroupInfo;
		}
		return null;
	}

	public override bool HasDataContent(Configuration config)
	{
		object[] array = new object[2] { Sections, Groups };
		for (int i = 0; i < array.Length; i++)
		{
			ConfigInfoCollection configInfoCollection = (ConfigInfoCollection)array[i];
			foreach (string item in configInfoCollection)
			{
				if (configInfoCollection[item].HasDataContent(config))
				{
					return true;
				}
			}
		}
		return false;
	}

	public override bool HasConfigContent(Configuration cfg)
	{
		if (base.StreamName == cfg.FileName)
		{
			return true;
		}
		object[] array = new object[2] { Sections, Groups };
		for (int i = 0; i < array.Length; i++)
		{
			ConfigInfoCollection configInfoCollection = (ConfigInfoCollection)array[i];
			foreach (string item in configInfoCollection)
			{
				if (configInfoCollection[item].HasConfigContent(cfg))
				{
					return true;
				}
			}
		}
		return false;
	}

	public override void ReadConfig(Configuration cfg, string streamName, XmlReader reader)
	{
		base.StreamName = streamName;
		ConfigHost = cfg.ConfigHost;
		if (reader.LocalName != "configSections")
		{
			while (reader.MoveToNextAttribute())
			{
				if (reader.Name == "name")
				{
					Name = reader.Value;
				}
				else if (reader.Name == "type")
				{
					TypeName = reader.Value;
					Type = null;
				}
				else
				{
					ThrowException("Unrecognized attribute", reader);
				}
			}
			if (Name == null)
			{
				ThrowException("sectionGroup must have a 'name' attribute", reader);
			}
			if (Name == "location")
			{
				ThrowException("location is a reserved section name", reader);
			}
		}
		if (TypeName == null)
		{
			TypeName = "System.Configuration.ConfigurationSectionGroup";
		}
		if (reader.IsEmptyElement)
		{
			reader.Skip();
			return;
		}
		reader.ReadStartElement();
		reader.MoveToContent();
		while (reader.NodeType != XmlNodeType.EndElement)
		{
			if (reader.NodeType != XmlNodeType.Element)
			{
				reader.Skip();
				continue;
			}
			string localName = reader.LocalName;
			ConfigInfo configInfo = null;
			switch (localName)
			{
			case "remove":
				ReadRemoveSection(reader);
				continue;
			case "clear":
				if (reader.HasAttributes)
				{
					ThrowException("Unrecognized attribute.", reader);
				}
				Clear();
				reader.Skip();
				continue;
			case "section":
				configInfo = new SectionInfo();
				break;
			case "sectionGroup":
				configInfo = new SectionGroupInfo();
				break;
			default:
				ThrowException("Unrecognized element: " + reader.Name, reader);
				break;
			}
			configInfo.ReadConfig(cfg, streamName, reader);
			ConfigInfo configInfo2 = Groups[configInfo.Name];
			if (configInfo2 == null)
			{
				configInfo2 = Sections[configInfo.Name];
			}
			if (configInfo2 != null)
			{
				if (configInfo2.GetType() != configInfo.GetType())
				{
					ThrowException("A section or section group named '" + configInfo.Name + "' already exists", reader);
				}
				configInfo2.Merge(configInfo);
				configInfo2.StreamName = streamName;
			}
			else
			{
				AddChild(configInfo);
			}
		}
		reader.ReadEndElement();
	}

	public override void WriteConfig(Configuration cfg, XmlWriter writer, ConfigurationSaveMode mode)
	{
		if (Name != null)
		{
			writer.WriteStartElement("sectionGroup");
			writer.WriteAttributeString("name", Name);
			if (TypeName != null && TypeName != "" && TypeName != "System.Configuration.ConfigurationSectionGroup")
			{
				writer.WriteAttributeString("type", TypeName);
			}
		}
		else
		{
			writer.WriteStartElement("configSections");
		}
		object[] array = new object[2] { Sections, Groups };
		for (int i = 0; i < array.Length; i++)
		{
			ConfigInfoCollection configInfoCollection = (ConfigInfoCollection)array[i];
			foreach (string item in configInfoCollection)
			{
				ConfigInfo configInfo = configInfoCollection[item];
				if (configInfo.HasConfigContent(cfg))
				{
					configInfo.WriteConfig(cfg, writer, mode);
				}
			}
		}
		writer.WriteEndElement();
	}

	private void ReadRemoveSection(XmlReader reader)
	{
		if (!reader.MoveToNextAttribute() || reader.Name != "name")
		{
			ThrowException("Unrecognized attribute.", reader);
		}
		string value = reader.Value;
		if (string.IsNullOrEmpty(value))
		{
			ThrowException("Empty name to remove", reader);
		}
		reader.MoveToElement();
		if (!HasChild(value))
		{
			ThrowException("No factory for " + value, reader);
		}
		RemoveChild(value);
		reader.Skip();
	}

	public void ReadRootData(XmlReader reader, Configuration config, bool overrideAllowed)
	{
		reader.MoveToContent();
		ReadContent(reader, config, overrideAllowed, root: true);
	}

	public override void ReadData(Configuration config, XmlReader reader, bool overrideAllowed)
	{
		reader.MoveToContent();
		if (!reader.IsEmptyElement)
		{
			reader.ReadStartElement();
			ReadContent(reader, config, overrideAllowed, root: false);
			reader.MoveToContent();
			reader.ReadEndElement();
		}
		else
		{
			reader.Read();
		}
	}

	private void ReadContent(XmlReader reader, Configuration config, bool overrideAllowed, bool root)
	{
		while (reader.NodeType != XmlNodeType.EndElement && reader.NodeType != 0)
		{
			if (reader.NodeType != XmlNodeType.Element)
			{
				reader.Skip();
			}
			else if (reader.LocalName == "dllmap")
			{
				reader.Skip();
			}
			else if (reader.LocalName == "location")
			{
				if (!root)
				{
					ThrowException("<location> elements are only allowed in <configuration> elements.", reader);
				}
				string attribute = reader.GetAttribute("allowOverride");
				bool flag = attribute == null || attribute.Length == 0 || bool.Parse(attribute);
				string attribute2 = reader.GetAttribute("path");
				if (attribute2 != null && attribute2.Length > 0)
				{
					string xmlContent = reader.ReadOuterXml();
					string[] array = attribute2.Split(',');
					for (int i = 0; i < array.Length; i++)
					{
						string text = array[i].Trim();
						if (config.Locations.Find(text) != null)
						{
							ThrowException("Sections must only appear once per config file.", reader);
						}
						ConfigurationLocation loc = new ConfigurationLocation(text, xmlContent, config, flag);
						config.Locations.Add(loc);
					}
				}
				else
				{
					ReadData(config, reader, flag);
				}
			}
			else
			{
				ConfigInfo configInfo = GetConfigInfo(reader, this);
				if (configInfo != null)
				{
					configInfo.ReadData(config, reader, overrideAllowed);
				}
				else
				{
					ThrowException("Unrecognized configuration section <" + reader.LocalName + ">", reader);
				}
			}
		}
	}

	private ConfigInfo GetConfigInfo(XmlReader reader, SectionGroupInfo current)
	{
		ConfigInfo configInfo = null;
		if (current.sections != null)
		{
			configInfo = current.sections[reader.LocalName];
		}
		if (configInfo != null)
		{
			return configInfo;
		}
		if (current.groups != null)
		{
			configInfo = current.groups[reader.LocalName];
		}
		if (configInfo != null)
		{
			return configInfo;
		}
		if (current.groups == null)
		{
			return null;
		}
		foreach (string allKey in current.groups.AllKeys)
		{
			configInfo = GetConfigInfo(reader, (SectionGroupInfo)current.groups[allKey]);
			if (configInfo != null)
			{
				return configInfo;
			}
		}
		return null;
	}

	internal override void Merge(ConfigInfo newData)
	{
		if (!(newData is SectionGroupInfo sectionGroupInfo))
		{
			return;
		}
		if (sectionGroupInfo.sections != null && sectionGroupInfo.sections.Count > 0)
		{
			foreach (string allKey in sectionGroupInfo.sections.AllKeys)
			{
				if (sections[allKey] == null)
				{
					sections.Add(allKey, sectionGroupInfo.sections[allKey]);
				}
			}
		}
		if (sectionGroupInfo.groups == null || sectionGroupInfo.sections == null || sectionGroupInfo.sections.Count <= 0)
		{
			return;
		}
		foreach (string allKey2 in sectionGroupInfo.groups.AllKeys)
		{
			if (groups[allKey2] == null)
			{
				groups.Add(allKey2, sectionGroupInfo.groups[allKey2]);
			}
		}
	}

	public void WriteRootData(XmlWriter writer, Configuration config, ConfigurationSaveMode mode)
	{
		WriteContent(writer, config, mode, writeElem: false);
	}

	public override void WriteData(Configuration config, XmlWriter writer, ConfigurationSaveMode mode)
	{
		writer.WriteStartElement(Name);
		WriteContent(writer, config, mode, writeElem: true);
		writer.WriteEndElement();
	}

	public void WriteContent(XmlWriter writer, Configuration config, ConfigurationSaveMode mode, bool writeElem)
	{
		object[] array = new object[2] { Sections, Groups };
		for (int i = 0; i < array.Length; i++)
		{
			ConfigInfoCollection configInfoCollection = (ConfigInfoCollection)array[i];
			foreach (string item in configInfoCollection)
			{
				ConfigInfo configInfo = configInfoCollection[item];
				if (configInfo.HasDataContent(config))
				{
					configInfo.WriteData(config, writer, mode);
				}
			}
		}
	}

	internal override bool HasValues(Configuration config, ConfigurationSaveMode mode)
	{
		if (modified && mode == ConfigurationSaveMode.Modified)
		{
			return true;
		}
		object[] array = new object[2] { Sections, Groups };
		for (int i = 0; i < array.Length; i++)
		{
			ConfigInfoCollection configInfoCollection = (ConfigInfoCollection)array[i];
			foreach (string item in configInfoCollection)
			{
				if (configInfoCollection[item].HasValues(config, mode))
				{
					return true;
				}
			}
		}
		return false;
	}

	internal override void ResetModified(Configuration config)
	{
		modified = false;
		object[] array = new object[2] { Sections, Groups };
		for (int i = 0; i < array.Length; i++)
		{
			ConfigInfoCollection configInfoCollection = (ConfigInfoCollection)array[i];
			foreach (string item in configInfoCollection)
			{
				configInfoCollection[item].ResetModified(config);
			}
		}
	}
}
