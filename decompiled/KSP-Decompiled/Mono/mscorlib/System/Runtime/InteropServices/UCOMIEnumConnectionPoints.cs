namespace System.Runtime.InteropServices;

[ComImport]
[Obsolete]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
[Guid("b196b285-bab4-101a-b69c-00aa00341d07")]
public interface UCOMIEnumConnectionPoints
{
	[PreserveSig]
	int Next(int celt, [Out][MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 0)] UCOMIConnectionPoint[] rgelt, out int pceltFetched);

	[PreserveSig]
	int Skip(int celt);

	[PreserveSig]
	int Reset();

	void Clone(out UCOMIEnumConnectionPoints ppenum);
}
