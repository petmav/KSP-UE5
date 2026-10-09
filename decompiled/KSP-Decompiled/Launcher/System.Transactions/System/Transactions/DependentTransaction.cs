using System.Runtime.Serialization;
using Unity;

namespace System.Transactions;

[Serializable]
[System.MonoTODO("Not supported yet")]
public sealed class DependentTransaction : Transaction, ISerializable
{
	private bool completed;

	internal bool Completed => completed;

	internal DependentTransaction(Transaction parent, DependentCloneOption option)
	{
	}

	[System.MonoTODO]
	public void Complete()
	{
		throw new NotImplementedException();
	}

	void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
		completed = info.GetBoolean("completed");
	}

	internal DependentTransaction()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
