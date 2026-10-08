namespace System.Runtime.InteropServices;

[Obsolete("Use System.Runtime.InteropServices.ComTypes.IEnumString instead. http://go.microsoft.com/fwlink/?linkid=14202", false)]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
[Guid("00000101-0000-0000-C000-000000000046")]
public interface UCOMIEnumString
{
	void Clone(out UCOMIEnumString ppenum);

	int Next(int celt, [Out][MarshalAs(UnmanagedType.LPArray, ArraySubType = (UnmanagedType)102)] string[] rgelt, out int pceltFetched);

	int Reset();

	int Skip(int celt);
}
