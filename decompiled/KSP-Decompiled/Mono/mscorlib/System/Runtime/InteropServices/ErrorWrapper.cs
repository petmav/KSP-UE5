using System.Security;
using System.Security.Permissions;

namespace System.Runtime.InteropServices;

[Serializable]
[ComVisible(true)]
public sealed class ErrorWrapper
{
	private int m_ErrorCode;

	public int ErrorCode => m_ErrorCode;

	public ErrorWrapper(int errorCode)
	{
		m_ErrorCode = errorCode;
	}

	public ErrorWrapper(object errorCode)
	{
		if (!(errorCode is int))
		{
			throw new ArgumentException(Environment.GetResourceString("Object must be of type Int32."), "errorCode");
		}
		m_ErrorCode = (int)errorCode;
	}

	[SecuritySafeCritical]
	[SecurityPermission(SecurityAction.Demand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public ErrorWrapper(Exception e)
	{
		m_ErrorCode = Marshal.GetHRForException(e);
	}
}
