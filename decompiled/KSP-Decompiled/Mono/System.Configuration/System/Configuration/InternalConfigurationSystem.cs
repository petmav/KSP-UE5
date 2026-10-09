using System.Configuration.Internal;

namespace System.Configuration;

internal class InternalConfigurationSystem : IConfigSystem
{
	private IInternalConfigHost host;

	private IInternalConfigRoot root;

	private object[] hostInitParams;

	public IInternalConfigHost Host => host;

	public IInternalConfigRoot Root => root;

	public void Init(Type typeConfigHost, params object[] hostInitParams)
	{
		this.hostInitParams = hostInitParams;
		host = (IInternalConfigHost)Activator.CreateInstance(typeConfigHost);
		root = new InternalConfigurationRoot();
		root.Init(host, isDesignTime: false);
	}

	public void InitForConfiguration(ref string locationConfigPath, out string parentConfigPath, out string parentLocationConfigPath)
	{
		host.InitForConfiguration(ref locationConfigPath, out parentConfigPath, out parentLocationConfigPath, root, hostInitParams);
	}
}
