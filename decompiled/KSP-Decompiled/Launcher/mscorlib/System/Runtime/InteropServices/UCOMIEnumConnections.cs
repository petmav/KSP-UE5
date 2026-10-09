namespace System.Runtime.InteropServices;

[Obsolete("Use System.Runtime.InteropServices.ComTypes.IEnumConnections instead. http://go.microsoft.com/fwlink/?linkid=14202", false)]
[Guid("B196B287-BAB4-101A-B69C-00AA00341D07")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface UCOMIEnumConnections
{
	void Clone(out UCOMIEnumConnections ppenum);

	int Next(int celt, [Out][MarshalAs(UnmanagedType.LPArray, ArraySubType = (UnmanagedType)102)] CONNECTDATA[] rgelt, out int pceltFetched);

	void Reset();

	int Skip(int celt);
}
