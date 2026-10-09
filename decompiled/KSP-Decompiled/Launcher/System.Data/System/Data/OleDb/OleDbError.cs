using Unity;

namespace System.Data.OleDb;

[Serializable]
public sealed class OleDbError
{
	public string Message
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public int NativeError
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public string Source
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string SQLState
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal OleDbError()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
