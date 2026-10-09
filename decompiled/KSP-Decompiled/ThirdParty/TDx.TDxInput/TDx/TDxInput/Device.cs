using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[Guid("CB3BF65E-0816-482A-BB11-64AF1E837812")]
	[CoClass(typeof(DeviceClass))]
	public interface Device : ISimpleDevice, _ISimpleDeviceEvents_Event
	{
	}
}
