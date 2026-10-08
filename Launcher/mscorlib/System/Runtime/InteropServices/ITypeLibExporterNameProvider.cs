namespace System.Runtime.InteropServices;

[ComVisible(true)]
[Guid("FA1F3615-ACB9-486d-9EAC-1BEF87E36B09")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface ITypeLibExporterNameProvider
{
	[return: MarshalAs(UnmanagedType.SafeArray)]
	string[] GetNames();
}
