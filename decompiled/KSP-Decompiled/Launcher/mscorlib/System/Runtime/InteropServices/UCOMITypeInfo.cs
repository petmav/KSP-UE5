namespace System.Runtime.InteropServices;

[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
[Obsolete("Use System.Runtime.InteropServices.ComTypes.ITypeInfo instead. http://go.microsoft.com/fwlink/?linkid=14202", false)]
[Guid("00020401-0000-0000-C000-000000000046")]
public interface UCOMITypeInfo
{
	void AddressOfMember(int memid, INVOKEKIND invKind, out IntPtr ppv);

	void CreateInstance([MarshalAs(UnmanagedType.IUnknown)] object pUnkOuter, ref Guid riid, [MarshalAs(UnmanagedType.IUnknown)] out object ppvObj);

	void GetContainingTypeLib(out UCOMITypeLib ppTLB, out int pIndex);

	void GetDllEntry(int memid, INVOKEKIND invKind, out string pBstrDllName, out string pBstrName, out short pwOrdinal);

	void GetDocumentation(int index, out string strName, out string strDocString, out int dwHelpContext, out string strHelpFile);

	void GetFuncDesc(int index, out IntPtr ppFuncDesc);

	void GetIDsOfNames([In][MarshalAs(UnmanagedType.LPArray, ArraySubType = (UnmanagedType)102)] string[] rgszNames, int cNames, [Out][MarshalAs(UnmanagedType.LPArray, ArraySubType = (UnmanagedType)102)] int[] pMemId);

	void GetImplTypeFlags(int index, out int pImplTypeFlags);

	void GetMops(int memid, out string pBstrMops);

	void GetNames(int memid, [Out][MarshalAs(UnmanagedType.LPArray, ArraySubType = (UnmanagedType)102)] string[] rgBstrNames, int cMaxNames, out int pcNames);

	void GetRefTypeInfo(int hRef, out UCOMITypeInfo ppTI);

	void GetRefTypeOfImplType(int index, out int href);

	void GetTypeAttr(out IntPtr ppTypeAttr);

	void GetTypeComp(out UCOMITypeComp ppTComp);

	void GetVarDesc(int index, out IntPtr ppVarDesc);

	void Invoke([MarshalAs(UnmanagedType.IUnknown)] object pvInstance, int memid, short wFlags, ref DISPPARAMS pDispParams, out object pVarResult, out EXCEPINFO pExcepInfo, out int puArgErr);

	void ReleaseFuncDesc(IntPtr pFuncDesc);

	void ReleaseTypeAttr(IntPtr pTypeAttr);

	void ReleaseVarDesc(IntPtr pVarDesc);
}
