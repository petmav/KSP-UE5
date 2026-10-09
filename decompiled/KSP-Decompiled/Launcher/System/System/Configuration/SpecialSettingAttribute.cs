using Unity;

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Property)]
public sealed class SpecialSettingAttribute : Attribute
{
	public SpecialSetting SpecialSetting
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SpecialSetting);
		}
	}

	public SpecialSettingAttribute(SpecialSetting specialSetting)
	{
	}
}
