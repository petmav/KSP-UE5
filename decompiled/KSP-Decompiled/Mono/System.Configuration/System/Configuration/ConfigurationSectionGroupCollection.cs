using System.Collections;
using System.Collections.Specialized;
using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Configuration;

[Serializable]
public sealed class ConfigurationSectionGroupCollection : NameObjectCollectionBase
{
	private SectionGroupInfo group;

	private Configuration config;

	public override KeysCollection Keys => group.Groups.Keys;

	public override int Count => group.Groups.Count;

	public ConfigurationSectionGroup this[string name]
	{
		get
		{
			ConfigurationSectionGroup configurationSectionGroup = BaseGet(name) as ConfigurationSectionGroup;
			if (configurationSectionGroup == null)
			{
				if (!(group.Groups[name] is SectionGroupInfo sectionGroupInfo))
				{
					return null;
				}
				configurationSectionGroup = config.GetSectionGroupInstance(sectionGroupInfo);
				BaseSet(name, configurationSectionGroup);
			}
			return configurationSectionGroup;
		}
	}

	public ConfigurationSectionGroup this[int index] => this[GetKey(index)];

	internal ConfigurationSectionGroupCollection(Configuration config, SectionGroupInfo group)
		: base(StringComparer.Ordinal)
	{
		this.config = config;
		this.group = group;
	}

	public void Add(string name, ConfigurationSectionGroup sectionGroup)
	{
		config.CreateSectionGroup(group, name, sectionGroup);
	}

	public void Clear()
	{
		if (group.Groups == null)
		{
			return;
		}
		foreach (ConfigInfo group in group.Groups)
		{
			config.RemoveConfigInfo(group);
		}
	}

	public void CopyTo(ConfigurationSectionGroup[] array, int index)
	{
		for (int i = 0; i < group.Groups.Count; i++)
		{
			array[i + index] = this[i];
		}
	}

	public ConfigurationSectionGroup Get(int index)
	{
		return this[index];
	}

	public ConfigurationSectionGroup Get(string name)
	{
		return this[name];
	}

	public override IEnumerator GetEnumerator()
	{
		return group.Groups.AllKeys.GetEnumerator();
	}

	public string GetKey(int index)
	{
		return group.Groups.GetKey(index);
	}

	public void Remove(string name)
	{
		if (group.Groups[name] is SectionGroupInfo sectionGroupInfo)
		{
			config.RemoveConfigInfo(sectionGroupInfo);
		}
	}

	public void RemoveAt(int index)
	{
		SectionGroupInfo sectionGroupInfo = group.Groups[index] as SectionGroupInfo;
		config.RemoveConfigInfo(sectionGroupInfo);
	}

	[System.MonoTODO]
	[SecurityPermission(SecurityAction.Demand, SerializationFormatter = true)]
	public override void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		throw new NotImplementedException();
	}

	internal ConfigurationSectionGroupCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
