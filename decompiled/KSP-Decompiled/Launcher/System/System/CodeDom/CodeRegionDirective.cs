using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeRegionDirective : CodeDirective
{
	public CodeRegionMode RegionMode
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(CodeRegionMode);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string RegionText
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

	public CodeRegionDirective()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeRegionDirective(CodeRegionMode regionMode, string regionText)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
