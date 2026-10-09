using System.Data.Common;
using Unity;

namespace System.Data.Odbc;

public sealed class OdbcFactory : DbProviderFactory
{
	public static readonly OdbcFactory Instance;

	internal OdbcFactory()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
