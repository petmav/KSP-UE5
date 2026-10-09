using System.Configuration.Internal;
using System.Text;
using System.Xml;

namespace System.Configuration;

internal abstract class ConfigInfo
{
	public string Name;

	public string TypeName;

	protected Type Type;

	private string streamName;

	public ConfigInfo Parent;

	public IInternalConfigHost ConfigHost;

	public string XPath
	{
		get
		{
			StringBuilder stringBuilder = new StringBuilder(Name);
			for (ConfigInfo parent = Parent; parent != null; parent = parent.Parent)
			{
				stringBuilder.Insert(0, parent.Name + "/");
			}
			return stringBuilder.ToString();
		}
	}

	public string StreamName
	{
		get
		{
			return streamName;
		}
		set
		{
			streamName = value;
		}
	}

	public virtual object CreateInstance()
	{
		if (Type == null)
		{
			Type = ConfigHost.GetConfigType(TypeName, throwOnError: true);
		}
		return Activator.CreateInstance(Type, nonPublic: true);
	}

	public abstract bool HasConfigContent(Configuration cfg);

	public abstract bool HasDataContent(Configuration cfg);

	protected void ThrowException(string text, XmlReader reader)
	{
		throw new ConfigurationErrorsException(text, reader);
	}

	public abstract void ReadConfig(Configuration cfg, string streamName, XmlReader reader);

	public abstract void WriteConfig(Configuration cfg, XmlWriter writer, ConfigurationSaveMode mode);

	public abstract void ReadData(Configuration config, XmlReader reader, bool overrideAllowed);

	public abstract void WriteData(Configuration config, XmlWriter writer, ConfigurationSaveMode mode);

	internal abstract void Merge(ConfigInfo data);

	internal abstract bool HasValues(Configuration config, ConfigurationSaveMode mode);

	internal abstract void ResetModified(Configuration config);
}
