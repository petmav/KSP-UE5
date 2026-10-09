namespace System.Runtime.InteropServices;

[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
[Obsolete("Use System.Runtime.InteropServices.ComTypes.IEnumConnectionPoints instead. http://go.microsoft.com/fwlink/?linkid=14202", false)]
[Guid("B196B285-BAB4-101A-B69C-00AA00341D07")]
public interface UCOMIEnumConnectionPoints
{
	void Clone(out UCOMIEnumConnectionPoints ppenum);

	int Next(int celt, [Out][MarshalAs(UnmanagedType.LPArray, ArraySubType = (UnmanagedType)102)] UCOMIConnectionPoint[] rgelt, out int pceltFetched);

	int Reset();

	int Skip(int celt);
}
