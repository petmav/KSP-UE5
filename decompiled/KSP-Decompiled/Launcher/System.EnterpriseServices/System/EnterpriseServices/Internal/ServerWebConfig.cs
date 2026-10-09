using Unity;

namespace System.EnterpriseServices.Internal;

public class ServerWebConfig : IServerWebConfig
{
	public ServerWebConfig()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddElement(string FilePath, string AssemblyName, string TypeName, string ProgId, string WkoMode, out string Error)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Create(string FilePath, string FilePrefix, out string Error)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
