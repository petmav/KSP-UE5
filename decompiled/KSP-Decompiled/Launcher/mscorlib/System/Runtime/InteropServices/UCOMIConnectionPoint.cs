namespace System.Runtime.InteropServices;

[Guid("B196B286-BAB4-101A-B69C-00AA00341D07")]
[Obsolete("Use System.Runtime.InteropServices.ComTypes.IConnectionPoint instead. http://go.microsoft.com/fwlink/?linkid=14202", false)]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface UCOMIConnectionPoint
{
	void Advise([MarshalAs(UnmanagedType.Interface)] object pUnkSink, out int pdwCookie);

	void EnumConnections(out UCOMIEnumConnections ppEnum);

	void GetConnectionInterface(out Guid pIID);

	void GetConnectionPointContainer(out UCOMIConnectionPointContainer ppCPC);

	void Unadvise(int dwCookie);
}
