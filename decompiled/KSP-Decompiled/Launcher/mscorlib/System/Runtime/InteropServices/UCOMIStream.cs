namespace System.Runtime.InteropServices;

[Guid("0000000c-0000-0000-C000-000000000046")]
[Obsolete("Use System.Runtime.InteropServices.ComTypes.IStream instead. http://go.microsoft.com/fwlink/?linkid=14202", false)]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface UCOMIStream
{
	void Clone(out UCOMIStream ppstm);

	void Commit(int grfCommitFlags);

	void CopyTo(UCOMIStream pstm, long cb, IntPtr pcbRead, IntPtr pcbWritten);

	void LockRegion(long libOffset, long cb, int dwLockType);

	void Read([Out][MarshalAs(UnmanagedType.LPArray, ArraySubType = (UnmanagedType)102)] byte[] pv, int cb, IntPtr pcbRead);

	void Revert();

	void Seek(long dlibMove, int dwOrigin, IntPtr plibNewPosition);

	void SetSize(long libNewSize);

	void Stat(out STATSTG pstatstg, int grfStatFlag);

	void UnlockRegion(long libOffset, long cb, int dwLockType);

	void Write([MarshalAs(UnmanagedType.LPArray, ArraySubType = (UnmanagedType)102)] byte[] pv, int cb, IntPtr pcbWritten);
}
