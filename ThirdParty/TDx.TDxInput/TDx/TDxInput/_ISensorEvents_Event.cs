using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComEventInterface(typeof(_ISensorEvents_0000), typeof(_ISensorEvents_EventProvider_0000))]
	[ComVisible(false)]
	[TypeLibType(16)]
	public interface _ISensorEvents_Event
	{
		event _ISensorEvents_SensorInputEventHandler SensorInput;
	}
}
