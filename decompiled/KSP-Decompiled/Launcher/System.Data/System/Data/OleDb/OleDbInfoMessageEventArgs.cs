using Unity;

namespace System.Data.OleDb;

public sealed class OleDbInfoMessageEventArgs : EventArgs
{
	public int ErrorCode
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public OleDbErrorCollection Errors
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

	public string Source
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal OleDbInfoMessageEventArgs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
