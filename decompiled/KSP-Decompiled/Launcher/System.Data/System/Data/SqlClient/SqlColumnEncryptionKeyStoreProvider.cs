using Unity;

namespace System.Data.SqlClient;

public abstract class SqlColumnEncryptionKeyStoreProvider
{
	protected SqlColumnEncryptionKeyStoreProvider()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public abstract byte[] DecryptColumnEncryptionKey(string masterKeyPath, string encryptionAlgorithm, byte[] encryptedColumnEncryptionKey);

	public abstract byte[] EncryptColumnEncryptionKey(string masterKeyPath, string encryptionAlgorithm, byte[] columnEncryptionKey);
}
