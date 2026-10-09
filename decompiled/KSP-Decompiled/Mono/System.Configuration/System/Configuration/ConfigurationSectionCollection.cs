using System.Collections;
using System.Collections.Specialized;
using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Configuration;

[Serializable]
public sealed class ConfigurationSectionCollection : NameObjectCollectionBase
{
	private SectionGroupInfo group;

	private Configuration config;

	private static readonly object lockObject = new object();

	public override KeysCollection Keys => group.Sections.Keys;

	public override int Count => group.Sections.Count;

	public ConfigurationSection this[string name]
	{
		get
		{
			ConfigurationSection configurationSection = BaseGet(name) as ConfigurationSection;
			if (configurationSection == null)
			{
				if (!(group.Sections[name] is SectionInfo sectionInfo))
				{
					return null;
				}
				configurationSection = config.GetSectionInstance(sectionInfo, createDefaultInstance: true);
				if (configurationSection == null)
				{
					return null;
				}
				lock (lockObject)
				{
					BaseSet(name, configurationSection);
				}
			}
			return configurationSection;
		}
	}

	public ConfigurationSection this[int index] => this[GetKey(index)];

	internal ConfigurationSectionCollection(Configuration config, SectionGroupInfo group)
		: base(StringComparer.Ordinal)
	{
		this.config = config;
		this.group = group;
	}

	public void Add(string name, ConfigurationSection section)
	{
		config.CreateSection(group, name, section);
	}

	public void Clear()
	{
		if (group.Sections == null)
		{
			return;
		}
		foreach (ConfigInfo section in group.Sections)
		{
			config.RemoveConfigInfo(section);
		}
	}

	public void CopyTo(ConfigurationSection[] array, int index)
	{
		for (int i = 0; i < group.Sections.Count; i++)
		{
			array[i + index] = this[i];
		}
	}

	public ConfigurationSection Get(int index)
	{
		return this[index];
	}

	public ConfigurationSection Get(string name)
	{
		return this[name];
	}

	public override IEnumerator GetEnumerator()
	{
		foreach (string allKey in group.Sections.AllKeys)
		{
			yield return this[allKey];
		}
	}

	public string GetKey(int index)
	{
		return group.Sections.GetKey(index);
	}

	public void Remove(string name)
	{
		if (group.Sections[name] is SectionInfo sectionInfo)
		{
			config.RemoveConfigInfo(sectionInfo);
		}
	}

	public void RemoveAt(int index)
	{
		SectionInfo sectionInfo = group.Sections[index] as SectionInfo;
		config.RemoveConfigInfo(sectionInfo);
	}

	[System.MonoTODO]
	[SecurityPermission(SecurityAction.Demand, SerializationFormatter = true)]
	public override void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		throw new NotImplementedException();
	}

	internal ConfigurationSectionCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
