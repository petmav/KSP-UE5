using System.Runtime.Versioning;
using Unity;

namespace System.Configuration;

public class ConfigurationSectionGroup
{
	public bool IsDeclarationRequired
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool IsDeclared
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public string Name
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string SectionGroupName
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

	public string Type
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public ConfigurationSectionGroup()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ForceDeclaration()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ForceDeclaration(bool force)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual bool ShouldSerializeSectionGroupInTargetVersion(FrameworkName targetFramework)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
