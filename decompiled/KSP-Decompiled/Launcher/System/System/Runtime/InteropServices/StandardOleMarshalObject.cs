using Unity;

namespace System.Runtime.InteropServices;

[ComVisible(true)]
public class StandardOleMarshalObject : MarshalByRefObject
{
	protected StandardOleMarshalObject()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
