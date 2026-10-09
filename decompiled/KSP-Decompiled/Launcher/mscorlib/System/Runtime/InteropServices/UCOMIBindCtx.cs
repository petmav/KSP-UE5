namespace System.Runtime.InteropServices;

[Obsolete("Use System.Runtime.InteropServices.ComTypes.IBindCtx instead. http://go.microsoft.com/fwlink/?linkid=14202", false)]
[Guid("0000000e-0000-0000-C000-000000000046")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface UCOMIBindCtx
{
	void EnumObjectParam(out UCOMIEnumString ppenum);

	void GetBindOptions(ref BIND_OPTS pbindopts);

	void GetObjectParam([MarshalAs(UnmanagedType.LPWStr)] string pszKey, [MarshalAs(UnmanagedType.Interface)] out object ppunk);

	void GetRunningObjectTable(out UCOMIRunningObjectTable pprot);

	void RegisterObjectBound([MarshalAs(UnmanagedType.Interface)] object punk);

	void RegisterObjectParam([MarshalAs(UnmanagedType.LPWStr)] string pszKey, [MarshalAs(UnmanagedType.Interface)] object punk);

	void ReleaseBoundObjects();

	void RevokeObjectBound([MarshalAs(UnmanagedType.Interface)] object punk);

	void RevokeObjectParam([MarshalAs(UnmanagedType.LPWStr)] string pszKey);

	void SetBindOptions([In] ref BIND_OPTS pbindopts);
}
