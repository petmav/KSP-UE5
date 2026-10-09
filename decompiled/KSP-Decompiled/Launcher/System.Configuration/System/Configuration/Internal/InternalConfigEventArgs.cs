using Unity;

namespace System.Configuration.Internal;

public sealed class InternalConfigEventArgs : EventArgs
{
	public string ConfigPath
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public InternalConfigEventArgs(string configPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
