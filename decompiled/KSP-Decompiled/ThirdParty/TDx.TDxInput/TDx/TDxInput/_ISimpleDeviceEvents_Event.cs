using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComVisible(false)]
	[ComEventInterface(typeof(_ISimpleDeviceEvents_0000), typeof(_ISimpleDeviceEvents_EventProvider_0000))]
	[TypeLibType(16)]
	public interface _ISimpleDeviceEvents_Event
	{
		event _ISimpleDeviceEvents_DeviceChangeEventHandler DeviceChange;
	}
}
