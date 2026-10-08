using System.Data.Common;
using Unity;

namespace System.Data.Odbc;

[Serializable]
public sealed class OdbcException : DbException
{
	public OdbcErrorCollection Errors
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal OdbcException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
