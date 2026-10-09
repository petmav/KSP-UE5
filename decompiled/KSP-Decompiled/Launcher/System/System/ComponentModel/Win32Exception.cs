using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;
using System.Security.Permissions;

namespace System.ComponentModel;

[Serializable]
[SuppressUnmanagedCodeSecurity]
[HostProtection(SecurityAction.LinkDemand, SharedState = true)]
public class Win32Exception : ExternalException, ISerializable
{
	private readonly int nativeErrorCode;

	private static bool s_ErrorMessagesInitialized = false;

	private static Dictionary<int, string> s_ErrorMessage = new Dictionary<int, string>();

	public int NativeErrorCode => nativeErrorCode;

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public Win32Exception()
		: this(Marshal.GetLastWin32Error())
	{
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public Win32Exception(int error)
		: this(error, GetErrorMessage(error))
	{
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public Win32Exception(int error, string message)
		: base(message)
	{
		nativeErrorCode = error;
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public Win32Exception(string message)
		: this(Marshal.GetLastWin32Error(), message)
	{
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public Win32Exception(string message, Exception innerException)
		: base(message, innerException)
	{
		nativeErrorCode = Marshal.GetLastWin32Error();
	}

	protected Win32Exception(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
		nativeErrorCode = info.GetInt32("NativeErrorCode");
	}

	[SecurityPermission(SecurityAction.Demand, SerializationFormatter = true)]
	public override void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		info.AddValue("NativeErrorCode", nativeErrorCode);
		base.GetObjectData(info, context);
	}

	internal static string GetErrorMessage(int error)
	{
		if (!s_ErrorMessagesInitialized)
		{
			InitializeErrorMessages();
		}
		if (s_ErrorMessage.TryGetValue(error, out var value))
		{
			return value;
		}
		return $"mono-io-layer-error ({error})";
	}

	private static void InitializeErrorMessages()
	{
		if (s_ErrorMessagesInitialized)
		{
			return;
		}
		lock (s_ErrorMessage)
		{
			if (!s_ErrorMessagesInitialized)
			{
				s_ErrorMessage.Add(0, "Success");
				s_ErrorMessage.Add(2, "Cannot find the specified file");
				s_ErrorMessage.Add(3, "Cannot find the specified file");
				s_ErrorMessage.Add(4, "Too many open files");
				s_ErrorMessage.Add(5, "Access denied");
				s_ErrorMessage.Add(6, "Invalid handle");
				s_ErrorMessage.Add(13, "Invalid data");
				s_ErrorMessage.Add(14, "Out of memory");
				s_ErrorMessage.Add(17, "Not same device");
				s_ErrorMessage.Add(18, "No more files");
				s_ErrorMessage.Add(24, "Bad length");
				s_ErrorMessage.Add(31, "General failure");
				s_ErrorMessage.Add(32, "Sharing violation");
				s_ErrorMessage.Add(33, "Lock violation");
				s_ErrorMessage.Add(50, "Operation not supported");
				s_ErrorMessage.Add(55, "Device does not exist");
				s_ErrorMessage.Add(87, "Invalid parameter");
				s_ErrorMessage.Add(120, "Call not implemented");
				s_ErrorMessage.Add(123, "Invalid name");
				s_ErrorMessage.Add(127, "Process not found");
				s_ErrorMessage.Add(183, "Already exists");
				s_ErrorMessage.Add(267, "Is a directory");
				s_ErrorMessage.Add(995, "Operation aborted");
				s_ErrorMessage.Add(6000, "Encryption failed");
				s_ErrorMessage.Add(10004, "interrupted");
				s_ErrorMessage.Add(10009, "Bad file number");
				s_ErrorMessage.Add(10013, "Access denied");
				s_ErrorMessage.Add(10014, "Bad address");
				s_ErrorMessage.Add(10022, "Invalid arguments");
				s_ErrorMessage.Add(10024, "Too many open files");
				s_ErrorMessage.Add(10035, "Operation on non-blocking socket would block");
				s_ErrorMessage.Add(10036, "Operation in progress");
				s_ErrorMessage.Add(10037, "Operation already in progress");
				s_ErrorMessage.Add(10038, "The descriptor is not a socket");
				s_ErrorMessage.Add(10039, "Destination address required");
				s_ErrorMessage.Add(10040, "Message too long");
				s_ErrorMessage.Add(10041, "Protocol wrong type for socket");
				s_ErrorMessage.Add(10042, "Protocol option not supported");
				s_ErrorMessage.Add(10043, "Protocol not supported");
				s_ErrorMessage.Add(10044, "Socket not supported");
				s_ErrorMessage.Add(10045, "Operation not supported");
				s_ErrorMessage.Add(10046, "Protocol family not supported");
				s_ErrorMessage.Add(10047, "An address incompatible with the requested protocol was used");
				s_ErrorMessage.Add(10048, "Address already in use");
				s_ErrorMessage.Add(10049, "The requested address is not valid in this context");
				s_ErrorMessage.Add(10050, "Network subsystem is down");
				s_ErrorMessage.Add(10051, "Network is unreachable");
				s_ErrorMessage.Add(10052, "Connection broken, keep-alive detected a problem");
				s_ErrorMessage.Add(10053, "An established connection was aborted in your host machine.");
				s_ErrorMessage.Add(10054, "Connection reset by peer");
				s_ErrorMessage.Add(10055, "Not enough buffer space is available");
				s_ErrorMessage.Add(10056, "Socket is already connected");
				s_ErrorMessage.Add(10057, "The socket is not connected");
				s_ErrorMessage.Add(10058, "The socket has been shut down");
				s_ErrorMessage.Add(10059, "Too many references: cannot splice");
				s_ErrorMessage.Add(10060, "Connection timed out");
				s_ErrorMessage.Add(10061, "Connection refused");
				s_ErrorMessage.Add(10062, "Too many symbolic links encountered");
				s_ErrorMessage.Add(10063, "File name too long");
				s_ErrorMessage.Add(10064, "Host is down");
				s_ErrorMessage.Add(10065, "No route to host");
				s_ErrorMessage.Add(10066, "Directory not empty");
				s_ErrorMessage.Add(10067, "EPROCLIM");
				s_ErrorMessage.Add(10068, "Too many users");
				s_ErrorMessage.Add(10069, "Quota exceeded");
				s_ErrorMessage.Add(10070, "Stale NFS file handle");
				s_ErrorMessage.Add(10071, "Object is remote");
				s_ErrorMessage.Add(10091, "SYSNOTREADY");
				s_ErrorMessage.Add(10092, "VERNOTSUPPORTED");
				s_ErrorMessage.Add(10093, "Winsock not initialised");
				s_ErrorMessage.Add(10101, "EDISCON");
				s_ErrorMessage.Add(10102, "ENOMORE");
				s_ErrorMessage.Add(10103, "Operation canceled");
				s_ErrorMessage.Add(10104, "EINVALIDPROCTABLE");
				s_ErrorMessage.Add(10105, "EINVALIDPROVIDER");
				s_ErrorMessage.Add(10106, "EPROVIDERFAILEDINIT");
				s_ErrorMessage.Add(10107, "System call failed");
				s_ErrorMessage.Add(10108, "SERVICE_NOT_FOUND");
				s_ErrorMessage.Add(10109, "TYPE_NOT_FOUND");
				s_ErrorMessage.Add(10112, "EREFUSED");
				s_ErrorMessage.Add(11001, "No such host is known");
				s_ErrorMessage.Add(11002, "A temporary error occurred on an authoritative name server.  Try again later.");
				s_ErrorMessage.Add(11003, "No recovery");
				s_ErrorMessage.Add(11004, "No data");
				s_ErrorMessagesInitialized = true;
			}
		}
	}
}
