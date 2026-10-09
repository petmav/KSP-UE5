using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.ConstrainedExecution;
using System.Runtime.InteropServices;
using System.Security.AccessControl;
using System.Security.Permissions;

namespace System.Threading;

[ComVisible(true)]
public sealed class Mutex : WaitHandle
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool ReleaseMutex_internal(IntPtr handle);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern IntPtr CreateMutex_internal(bool initiallyOwned, string name, out bool created);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern IntPtr OpenMutex_internal(string name, MutexRights rights, out MonoIOError error);

	private Mutex(IntPtr handle)
	{
		Handle = handle;
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public Mutex()
	{
		Handle = CreateMutex_internal(initiallyOwned: false, null, out var _);
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public Mutex(bool initiallyOwned)
	{
		Handle = CreateMutex_internal(initiallyOwned, null, out var _);
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	[SecurityPermission(SecurityAction.LinkDemand, UnmanagedCode = true)]
	public Mutex(bool initiallyOwned, string name)
	{
		Handle = CreateMutex_internal(initiallyOwned, name, out var _);
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	[SecurityPermission(SecurityAction.LinkDemand, UnmanagedCode = true)]
	public Mutex(bool initiallyOwned, string name, out bool createdNew)
	{
		Handle = CreateMutex_internal(initiallyOwned, name, out createdNew);
	}

	[MonoTODO("Use MutexSecurity in CreateMutex_internal")]
	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public Mutex(bool initiallyOwned, string name, out bool createdNew, MutexSecurity mutexSecurity)
	{
		Handle = CreateMutex_internal(initiallyOwned, name, out createdNew);
	}

	public MutexSecurity GetAccessControl()
	{
		return new MutexSecurity(base.SafeWaitHandle, AccessControlSections.Access | AccessControlSections.Owner | AccessControlSections.Group);
	}

	public static Mutex OpenExisting(string name)
	{
		return OpenExisting(name, MutexRights.Modify | MutexRights.Synchronize);
	}

	public unsafe static Mutex OpenExisting(string name, MutexRights rights)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (name.Length == 0 || name.Length > 260)
		{
			throw new ArgumentException("name", Locale.GetText("Invalid length [1-260]."));
		}
		MonoIOError error;
		IntPtr intPtr = OpenMutex_internal(name, rights, out error);
		if (intPtr == (IntPtr)(void*)null)
		{
			switch (error)
			{
			case MonoIOError.ERROR_FILE_NOT_FOUND:
				throw new WaitHandleCannotBeOpenedException(Locale.GetText("Named Mutex handle does not exist: ") + name);
			case MonoIOError.ERROR_ACCESS_DENIED:
				throw new UnauthorizedAccessException();
			default:
				throw new IOException(Locale.GetText("Win32 IO error: ") + error);
			}
		}
		return new Mutex(intPtr);
	}

	public static bool TryOpenExisting(string name, out Mutex result)
	{
		return TryOpenExisting(name, MutexRights.Modify | MutexRights.Synchronize, out result);
	}

	public unsafe static bool TryOpenExisting(string name, MutexRights rights, out Mutex result)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (name.Length == 0 || name.Length > 260)
		{
			throw new ArgumentException("name", Locale.GetText("Invalid length [1-260]."));
		}
		MonoIOError error;
		IntPtr intPtr = OpenMutex_internal(name, rights, out error);
		if (intPtr == (IntPtr)(void*)null)
		{
			result = null;
			return false;
		}
		result = new Mutex(intPtr);
		return true;
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public void ReleaseMutex()
	{
		if (!ReleaseMutex_internal(Handle))
		{
			throw new ApplicationException("Mutex is not owned");
		}
	}

	public void SetAccessControl(MutexSecurity mutexSecurity)
	{
		if (mutexSecurity == null)
		{
			throw new ArgumentNullException("mutexSecurity");
		}
		mutexSecurity.PersistModifications(base.SafeWaitHandle);
	}
}
