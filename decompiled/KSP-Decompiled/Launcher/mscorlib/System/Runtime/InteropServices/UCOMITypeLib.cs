namespace System.Runtime.InteropServices;

[Obsolete("Use System.Runtime.InteropServices.ComTypes.ITypeLib instead. http://go.microsoft.com/fwlink/?linkid=14202", false)]
[Guid("00020402-0000-0000-C000-000000000046")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface UCOMITypeLib
{
	void FindName([MarshalAs(UnmanagedType.LPWStr)] string szNameBuf, int lHashVal, [Out][MarshalAs(UnmanagedType.LPArray, ArraySubType = (UnmanagedType)102)] UCOMITypeInfo[] ppTInfo, [Out][MarshalAs(UnmanagedType.LPArray, ArraySubType = (UnmanagedType)102)] int[] rgMemId, ref short pcFound);

	void GetDocumentation(int index, out string strName, out string strDocString, out int dwHelpContext, out string strHelpFile);

	void GetLibAttr(out IntPtr ppTLibAttr);

	void GetTypeComp(out UCOMITypeComp ppTComp);

	void GetTypeInfo(int index, out UCOMITypeInfo ppTI);

	int GetTypeInfoCount();

	void GetTypeInfoOfGuid(ref Guid guid, out UCOMITypeInfo ppTInfo);

	void GetTypeInfoType(int index, out TYPEKIND pTKind);

	[return: MarshalAs(UnmanagedType.Bool)]
	bool IsName([MarshalAs(UnmanagedType.LPWStr)] string szNameBuf, int lHashVal);

	void ReleaseTLibAttr(IntPtr pTLibAttr);
}
