using Unity;

namespace System.Data.Odbc;

public sealed class OdbcInfoMessageEventArgs : EventArgs
{
	public OdbcErrorCollection Errors
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string Message
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal OdbcInfoMessageEventArgs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
