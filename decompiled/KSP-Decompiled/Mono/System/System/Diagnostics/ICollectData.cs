using System.Runtime.InteropServices;

namespace System.Diagnostics;

[ComImport]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
[Guid("73386977-D6FD-11D2-BED5-00C04F79E3AE")]
public interface ICollectData
{
	void CloseData();

	[return: MarshalAs(UnmanagedType.I4)]
	void CollectData([In][MarshalAs(UnmanagedType.I4)] int id, [In][MarshalAs(UnmanagedType.SysInt)] IntPtr valueName, [In][MarshalAs(UnmanagedType.SysInt)] IntPtr data, [In][MarshalAs(UnmanagedType.I4)] int totalBytes, [MarshalAs(UnmanagedType.SysInt)] out IntPtr res);
}
