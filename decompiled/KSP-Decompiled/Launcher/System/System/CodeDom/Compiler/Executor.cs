using System.Security.Permissions;
using Unity;

namespace System.CodeDom.Compiler;

[PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
public static class Executor
{
	public static void ExecWait(string cmd, TempFileCollection tempFiles)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static int ExecWaitWithCapture(IntPtr userToken, string cmd, TempFileCollection tempFiles, ref string outputName, ref string errorName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public static int ExecWaitWithCapture(IntPtr userToken, string cmd, string currentDir, TempFileCollection tempFiles, ref string outputName, ref string errorName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public static int ExecWaitWithCapture(string cmd, TempFileCollection tempFiles, ref string outputName, ref string errorName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public static int ExecWaitWithCapture(string cmd, string currentDir, TempFileCollection tempFiles, ref string outputName, ref string errorName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}
}
