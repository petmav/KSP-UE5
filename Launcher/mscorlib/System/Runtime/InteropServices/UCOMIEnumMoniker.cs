namespace System.Runtime.InteropServices;

[Obsolete("Use System.Runtime.InteropServices.ComTypes.IEnumMoniker instead. http://go.microsoft.com/fwlink/?linkid=14202", false)]
[Guid("00000102-0000-0000-C000-000000000046")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface UCOMIEnumMoniker
{
	void Clone(out UCOMIEnumMoniker ppenum);

	int Next(int celt, [Out][MarshalAs(UnmanagedType.LPArray, ArraySubType = (UnmanagedType)102)] UCOMIMoniker[] rgelt, out int pceltFetched);

	int Reset();

	int Skip(int celt);
}
