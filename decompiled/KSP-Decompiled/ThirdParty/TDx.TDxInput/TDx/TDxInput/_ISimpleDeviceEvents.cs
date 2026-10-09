using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[Guid("8FE3A216-E235-49A6-9136-F9D81FDADEF5")]
	[InterfaceType(2)]
	[TypeLibType(4096)]
	public interface _ISimpleDeviceEvents
	{
		[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
		[DispId(1)]
		void DeviceChange([In] int reserved);
	}
}
