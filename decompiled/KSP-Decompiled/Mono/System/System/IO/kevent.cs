using System.Runtime.InteropServices;

namespace System.IO;

internal struct kevent : IDisposable
{
	public UIntPtr ident;

	public EventFilter filter;

	public EventFlags flags;

	public FilterFlags fflags;

	public IntPtr data;

	public IntPtr udata;

	public void Dispose()
	{
		if (udata != IntPtr.Zero)
		{
			Marshal.FreeHGlobal(udata);
		}
	}
}
