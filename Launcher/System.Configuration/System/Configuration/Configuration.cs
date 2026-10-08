using System.Runtime.Versioning;
using System.Security.Permissions;
using Unity;

namespace System.Configuration;

public sealed class Configuration
{
	public AppSettingsSection AppSettings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public Func<string, string> AssemblyStringTransformer
	{
		get
		{
			//IL_0007: Expected O, but got I4
			Unity.ThrowStub.ThrowNotSupportedException();
			return (Func<string, string>)0;
		}
		[ConfigurationPermission(SecurityAction.Demand, Unrestricted = true)]
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public ConnectionStringsSection ConnectionStrings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ContextInformation EvaluationContext
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string FilePath
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool HasFile
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public ConfigurationLocationCollection Locations
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool NamespaceDeclared
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public ConfigurationSectionGroup RootSectionGroup
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConfigurationSectionGroupCollection SectionGroups
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConfigurationSectionCollection Sections
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public FrameworkName TargetFramework
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		[ConfigurationPermission(SecurityAction.Demand, Unrestricted = true)]
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public Func<string, string> TypeStringTransformer
	{
		get
		{
			//IL_0007: Expected O, but got I4
			Unity.ThrowStub.ThrowNotSupportedException();
			return (Func<string, string>)0;
		}
		[ConfigurationPermission(SecurityAction.Demand, Unrestricted = true)]
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	internal Configuration()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationSection GetSection(string sectionName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public ConfigurationSectionGroup GetSectionGroup(string sectionGroupName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Save()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Save(ConfigurationSaveMode saveMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Save(ConfigurationSaveMode saveMode, bool forceSaveAll)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SaveAs(string filename)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SaveAs(string filename, ConfigurationSaveMode saveMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SaveAs(string filename, ConfigurationSaveMode saveMode, bool forceSaveAll)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
