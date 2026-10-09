using Unity;

namespace System.Data.Odbc;

[Serializable]
public sealed class OdbcError
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

	internal OdbcError()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
