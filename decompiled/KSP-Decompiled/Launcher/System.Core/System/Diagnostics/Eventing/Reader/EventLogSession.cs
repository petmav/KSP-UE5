using System.Collections.Generic;
using System.Globalization;
using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.Eventing.Reader;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class EventLogSession : IDisposable
{
	public static EventLogSession GlobalSession
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	[SecurityCritical]
	public EventLogSession()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogSession(string server)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityCritical]
	public EventLogSession(string server, string domain, string user, SecureString password, SessionAuthentication logOnType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CancelCurrentOperations()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearLog(string logName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearLog(string logName, string backupPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	protected virtual void Dispose(bool disposing)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ExportLog(string path, PathType pathType, string query, string targetFilePath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ExportLog(string path, PathType pathType, string query, string targetFilePath, bool tolerateQueryErrors)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ExportLogAndMessages(string path, PathType pathType, string query, string targetFilePath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ExportLogAndMessages(string path, PathType pathType, string query, string targetFilePath, bool tolerateQueryErrors, CultureInfo targetCultureInfo)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogInformation GetLogInformation(string logName, PathType pathType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecurityCritical]
	public IEnumerable<string> GetLogNames()
	{
		//IL_0007: Expected O, but got I4
		Unity.ThrowStub.ThrowNotSupportedException();
		return (IEnumerable<string>)0;
	}

	[SecurityCritical]
	public IEnumerable<string> GetProviderNames()
	{
		//IL_0007: Expected O, but got I4
		Unity.ThrowStub.ThrowNotSupportedException();
		return (IEnumerable<string>)0;
	}
}
