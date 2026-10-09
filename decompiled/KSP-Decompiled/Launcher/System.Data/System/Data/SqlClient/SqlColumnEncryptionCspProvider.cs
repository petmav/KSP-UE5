using Unity;

namespace System.Data.SqlClient;

public class SqlColumnEncryptionCspProvider : SqlColumnEncryptionKeyStoreProvider
{
	public const string ProviderName = "MSSQL_CSP_PROVIDER";

	public SqlColumnEncryptionCspProvider()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override byte[] DecryptColumnEncryptionKey(string masterKeyPath, string encryptionAlgorithm, byte[] encryptedColumnEncryptionKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override byte[] EncryptColumnEncryptionKey(string masterKeyPath, string encryptionAlgorithm, byte[] columnEncryptionKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
