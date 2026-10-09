using System.Collections;

namespace System.Configuration;

public class ConfigurationLocationCollection : ReadOnlyCollectionBase
{
	public ConfigurationLocation this[int index] => base.InnerList[index] as ConfigurationLocation;

	internal ConfigurationLocationCollection()
	{
	}

	internal void Add(ConfigurationLocation loc)
	{
		base.InnerList.Add(loc);
	}

	internal ConfigurationLocation Find(string location)
	{
		foreach (ConfigurationLocation inner in base.InnerList)
		{
			if (string.Compare(inner.Path, location, StringComparison.OrdinalIgnoreCase) == 0)
			{
				return inner;
			}
		}
		return null;
	}

	internal ConfigurationLocation FindBest(string location)
	{
		if (string.IsNullOrEmpty(location))
		{
			return null;
		}
		ConfigurationLocation configurationLocation = null;
		int length = location.Length;
		int num = 0;
		foreach (ConfigurationLocation inner in base.InnerList)
		{
			string path = inner.Path;
			if (string.IsNullOrEmpty(path))
			{
				continue;
			}
			int length2 = path.Length;
			if (!location.StartsWith(path, StringComparison.OrdinalIgnoreCase))
			{
				continue;
			}
			if (length == length2)
			{
				return inner;
			}
			if (length <= length2 || location[length2] == '/')
			{
				if (configurationLocation == null)
				{
					configurationLocation = inner;
				}
				else if (num < length2)
				{
					configurationLocation = inner;
					num = length2;
				}
			}
		}
		return configurationLocation;
	}
}
