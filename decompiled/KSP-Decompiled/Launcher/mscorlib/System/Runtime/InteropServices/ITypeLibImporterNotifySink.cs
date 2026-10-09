using System.Reflection;

namespace System.Runtime.InteropServices;

[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
[ComVisible(true)]
[Guid("F1C3BF76-C3E4-11d3-88E7-00902754C43A")]
public interface ITypeLibImporterNotifySink
{
	void ReportEvent(ImporterEventKind eventKind, int eventCode, string eventMsg);

	Assembly ResolveRef([MarshalAs(UnmanagedType.Interface)] object typeLib);
}
