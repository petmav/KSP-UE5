using Unity;

namespace System.Data.Common;

public static class DbProviderFactories
{
	public static DbProviderFactory GetFactory(DbConnection connection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static DbProviderFactory GetFactory(DataRow providerRow)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static DbProviderFactory GetFactory(string providerInvariantName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static DataTable GetFactoryClasses()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
