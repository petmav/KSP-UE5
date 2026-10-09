using System.Data.Common;
using Unity;

namespace System.Data.OleDb;

public sealed class OleDbFactory : DbProviderFactory
{
	public static readonly OleDbFactory Instance;

	internal OleDbFactory()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
