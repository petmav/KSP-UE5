using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices.Internal;

[Guid("d8013eef-730b-45e2-ba24-874b7242c425")]
public class Publish : IComSoapPublisher
{
	public Publish()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CreateMailBox(string RootMailServer, string MailBox, out string SmtpName, out string Domain, out string PhysicalPath, out string Error)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CreateVirtualRoot(string Operation, string FullUrl, out string BaseUrl, out string VirtualRoot, out string PhysicalPath, out string Error)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DeleteMailBox(string RootMailServer, string MailBox, out string Error)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DeleteVirtualRoot(string RootWebServer, string FullUrl, out string Error)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void GacInstall(string AssemblyPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void GacRemove(string AssemblyPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void GetAssemblyNameForCache(string TypeLibPath, out string CachePath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static string GetClientPhysicalPath(bool CreateDir)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public string GetTypeNameFromProgId(string AssemblyPath, string ProgId)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static void ParseUrl(string FullUrl, out string BaseUrl, out string VirtualRoot)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ProcessClientTlb(string ProgId, string SrcTlbPath, string PhysicalPath, string VRoot, string BaseUrl, string Mode, string Transport, out string AssemblyName, out string TypeName, out string Error)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ProcessServerTlb(string ProgId, string SrcTlbPath, string PhysicalPath, string Operation, out string strAssemblyName, out string TypeName, out string Error)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void RegisterAssembly(string AssemblyPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void UnRegisterAssembly(string AssemblyPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
