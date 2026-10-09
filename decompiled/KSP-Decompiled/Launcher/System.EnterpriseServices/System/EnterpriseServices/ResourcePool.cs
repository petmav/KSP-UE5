using Unity;

namespace System.EnterpriseServices;

public sealed class ResourcePool
{
	public delegate void TransactionEndDelegate(object resource);

	public ResourcePool(TransactionEndDelegate cb)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object GetResource()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public bool PutResource(object resource)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
