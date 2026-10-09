using System.Data.Common;
using Unity;

namespace System.Data.OleDb;

[Serializable]
public sealed class OleDbException : DbException
{
	public OleDbErrorCollection Errors
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal OleDbException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
