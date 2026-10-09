using System.IO;
using System.Runtime.Versioning;
using System.Xml;
using Unity;

namespace System.Configuration;

public abstract class ConfigurationSection : ConfigurationElement
{
	private SectionInformation sectionInformation;

	private IConfigurationSectionHandler section_handler;

	private string externalDataXml;

	private object _configContext;

	internal string ExternalDataXml => externalDataXml;

	internal IConfigurationSectionHandler SectionHandler
	{
		get
		{
			return section_handler;
		}
		set
		{
			section_handler = value;
		}
	}

	[System.MonoTODO]
	public SectionInformation SectionInformation
	{
		get
		{
			if (sectionInformation == null)
			{
				sectionInformation = new SectionInformation();
			}
			return sectionInformation;
		}
	}

	internal object ConfigContext
	{
		get
		{
			return _configContext;
		}
		set
		{
			_configContext = value;
		}
	}

	[System.MonoTODO("Provide ConfigContext. Likely the culprit of bug #322493")]
	protected internal virtual object GetRuntimeObject()
	{
		if (SectionHandler != null)
		{
			object obj = ((sectionInformation != null) ? sectionInformation.GetParentSection() : null)?.GetRuntimeObject();
			if (base.RawXml == null)
			{
				return obj;
			}
			try
			{
				XmlReader reader = new ConfigXmlTextReader(new StringReader(base.RawXml), base.Configuration.FilePath);
				DoDeserializeSection(reader);
				if (!string.IsNullOrEmpty(SectionInformation.ConfigSource))
				{
					string configFilePath = SectionInformation.ConfigFilePath;
					configFilePath = (string.IsNullOrEmpty(configFilePath) ? string.Empty : Path.GetDirectoryName(configFilePath));
					string path = Path.Combine(configFilePath, SectionInformation.ConfigSource);
					if (File.Exists(path))
					{
						base.RawXml = File.ReadAllText(path);
						SectionInformation.SetRawXml(base.RawXml);
					}
				}
			}
			catch
			{
			}
			XmlDocument xmlDocument = new ConfigurationXmlDocument();
			xmlDocument.LoadXml(base.RawXml);
			return SectionHandler.Create(obj, ConfigContext, xmlDocument.DocumentElement);
		}
		return this;
	}

	[System.MonoTODO]
	protected internal override bool IsModified()
	{
		return base.IsModified();
	}

	[System.MonoTODO]
	protected internal override void ResetModified()
	{
		base.ResetModified();
	}

	private ConfigurationElement CreateElement(Type t)
	{
		ConfigurationElement configurationElement = (ConfigurationElement)Activator.CreateInstance(t);
		configurationElement.Init();
		configurationElement.Configuration = base.Configuration;
		if (IsReadOnly())
		{
			configurationElement.SetReadOnly();
		}
		return configurationElement;
	}

	private void DoDeserializeSection(XmlReader reader)
	{
		reader.MoveToContent();
		string text = null;
		string text2 = null;
		while (reader.MoveToNextAttribute())
		{
			string localName = reader.LocalName;
			if (localName == "configProtectionProvider")
			{
				text = reader.Value;
			}
			else if (localName == "configSource")
			{
				text2 = reader.Value;
			}
		}
		if (text != null)
		{
			ProtectedConfigurationProvider provider = ProtectedConfiguration.GetProvider(text, throwOnError: true);
			XmlDocument xmlDocument = new ConfigurationXmlDocument();
			reader.MoveToElement();
			xmlDocument.Load(new StringReader(reader.ReadInnerXml()));
			reader = new XmlNodeReader(provider.Decrypt(xmlDocument));
			SectionInformation.ProtectSection(text);
			reader.MoveToContent();
		}
		if (text2 != null)
		{
			SectionInformation.ConfigSource = text2;
		}
		SectionInformation.SetRawXml(base.RawXml);
		if (SectionHandler == null)
		{
			DeserializeElement(reader, serializeCollectionKey: false);
		}
	}

	[System.MonoInternalNote("find the proper location for the decryption stuff")]
	protected internal virtual void DeserializeSection(XmlReader reader)
	{
		try
		{
			DoDeserializeSection(reader);
		}
		catch (ConfigurationErrorsException ex)
		{
			throw new ConfigurationErrorsException($"Error deserializing configuration section {SectionInformation.Name}: {ex.Message}");
		}
	}

	internal void DeserializeConfigSource(string basePath)
	{
		string configSource = SectionInformation.ConfigSource;
		if (!string.IsNullOrEmpty(configSource))
		{
			if (Path.IsPathRooted(configSource))
			{
				throw new ConfigurationErrorsException("The configSource attribute must be a relative physical path.");
			}
			if (HasLocalModifications())
			{
				throw new ConfigurationErrorsException("A section using 'configSource' may contain no other attributes or elements.");
			}
			string text = Path.Combine(basePath, configSource);
			if (!File.Exists(text))
			{
				base.RawXml = null;
				SectionInformation.SetRawXml(null);
				throw new ConfigurationErrorsException($"Unable to open configSource file '{text}'.");
			}
			base.RawXml = File.ReadAllText(text);
			SectionInformation.SetRawXml(base.RawXml);
			DeserializeElement(new ConfigXmlTextReader(new StringReader(base.RawXml), text), serializeCollectionKey: false);
		}
	}

	protected internal virtual string SerializeSection(ConfigurationElement parentElement, string name, ConfigurationSaveMode saveMode)
	{
		externalDataXml = null;
		ConfigurationElement configurationElement;
		if (parentElement != null)
		{
			configurationElement = CreateElement(GetType());
			configurationElement.Unmerge(this, parentElement, saveMode);
		}
		else
		{
			configurationElement = this;
		}
		configurationElement.PrepareSave(parentElement, saveMode);
		bool flag = configurationElement.HasValues(parentElement, saveMode);
		string result;
		using (StringWriter stringWriter = new StringWriter())
		{
			using (XmlTextWriter xmlTextWriter = new XmlTextWriter(stringWriter))
			{
				xmlTextWriter.Formatting = Formatting.Indented;
				if (flag)
				{
					configurationElement.SerializeToXmlElement(xmlTextWriter, name);
				}
				else if (saveMode == ConfigurationSaveMode.Modified && configurationElement.IsModified())
				{
					xmlTextWriter.WriteStartElement(name);
					xmlTextWriter.WriteEndElement();
				}
				xmlTextWriter.Close();
			}
			result = stringWriter.ToString();
		}
		string configSource = SectionInformation.ConfigSource;
		if (string.IsNullOrEmpty(configSource))
		{
			return result;
		}
		externalDataXml = result;
		using StringWriter stringWriter2 = new StringWriter();
		bool flag2 = !string.IsNullOrEmpty(name);
		using (XmlTextWriter xmlTextWriter2 = new XmlTextWriter(stringWriter2))
		{
			if (flag2)
			{
				xmlTextWriter2.WriteStartElement(name);
			}
			xmlTextWriter2.WriteAttributeString("configSource", configSource);
			if (flag2)
			{
				xmlTextWriter2.WriteEndElement();
			}
		}
		return stringWriter2.ToString();
	}

	protected internal virtual bool ShouldSerializeElementInTargetVersion(ConfigurationElement element, string elementName, FrameworkName targetFramework)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected internal virtual bool ShouldSerializePropertyInTargetVersion(ConfigurationProperty property, string propertyName, FrameworkName targetFramework, ConfigurationElement parentConfigurationElement)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected internal virtual bool ShouldSerializeSectionInTargetVersion(FrameworkName targetFramework)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
