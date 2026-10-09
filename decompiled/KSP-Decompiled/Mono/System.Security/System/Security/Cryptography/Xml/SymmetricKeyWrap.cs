namespace System.Security.Cryptography.Xml;

internal static class SymmetricKeyWrap
{
	private static readonly byte[] s_rgbTripleDES_KW_IV = new byte[8] { 74, 221, 162, 44, 121, 232, 33, 5 };

	private static readonly byte[] s_rgbAES_KW_IV = new byte[8] { 166, 166, 166, 166, 166, 166, 166, 166 };

	internal static byte[] TripleDESKeyWrapEncrypt(byte[] rgbKey, byte[] rgbWrappedKeyData)
	{
		byte[] src;
		using (SHA1 sHA = SHA1.Create())
		{
			src = sHA.ComputeHash(rgbWrappedKeyData);
		}
		byte[] array = new byte[8];
		using (RandomNumberGenerator randomNumberGenerator = RandomNumberGenerator.Create())
		{
			randomNumberGenerator.GetBytes(array);
		}
		byte[] array2 = new byte[rgbWrappedKeyData.Length + 8];
		TripleDES tripleDES = null;
		ICryptoTransform cryptoTransform = null;
		ICryptoTransform cryptoTransform2 = null;
		try
		{
			tripleDES = TripleDES.Create();
			tripleDES.Padding = PaddingMode.None;
			cryptoTransform = tripleDES.CreateEncryptor(rgbKey, array);
			cryptoTransform2 = tripleDES.CreateEncryptor(rgbKey, s_rgbTripleDES_KW_IV);
			Buffer.BlockCopy(rgbWrappedKeyData, 0, array2, 0, rgbWrappedKeyData.Length);
			Buffer.BlockCopy(src, 0, array2, rgbWrappedKeyData.Length, 8);
			byte[] array3 = cryptoTransform.TransformFinalBlock(array2, 0, array2.Length);
			byte[] array4 = new byte[array.Length + array3.Length];
			Buffer.BlockCopy(array, 0, array4, 0, array.Length);
			Buffer.BlockCopy(array3, 0, array4, array.Length, array3.Length);
			Array.Reverse(array4);
			return cryptoTransform2.TransformFinalBlock(array4, 0, array4.Length);
		}
		finally
		{
			cryptoTransform2?.Dispose();
			cryptoTransform?.Dispose();
			tripleDES?.Dispose();
		}
	}

	internal static byte[] TripleDESKeyWrapDecrypt(byte[] rgbKey, byte[] rgbEncryptedWrappedKeyData)
	{
		if (rgbEncryptedWrappedKeyData.Length != 32 && rgbEncryptedWrappedKeyData.Length != 40 && rgbEncryptedWrappedKeyData.Length != 48)
		{
			throw new CryptographicException("The length of the encrypted data in Key Wrap is either 32, 40 or 48 bytes.");
		}
		TripleDES tripleDES = null;
		ICryptoTransform cryptoTransform = null;
		ICryptoTransform cryptoTransform2 = null;
		try
		{
			tripleDES = TripleDES.Create();
			tripleDES.Padding = PaddingMode.None;
			cryptoTransform = tripleDES.CreateDecryptor(rgbKey, s_rgbTripleDES_KW_IV);
			byte[] array = cryptoTransform.TransformFinalBlock(rgbEncryptedWrappedKeyData, 0, rgbEncryptedWrappedKeyData.Length);
			Array.Reverse(array);
			byte[] array2 = new byte[8];
			Buffer.BlockCopy(array, 0, array2, 0, 8);
			byte[] array3 = new byte[array.Length - array2.Length];
			Buffer.BlockCopy(array, 8, array3, 0, array3.Length);
			cryptoTransform2 = tripleDES.CreateDecryptor(rgbKey, array2);
			byte[] array4 = cryptoTransform2.TransformFinalBlock(array3, 0, array3.Length);
			byte[] array5 = new byte[array4.Length - 8];
			Buffer.BlockCopy(array4, 0, array5, 0, array5.Length);
			using SHA1 sHA = SHA1.Create();
			byte[] array6 = sHA.ComputeHash(array5);
			int num = array5.Length;
			int num2 = 0;
			while (num < array4.Length)
			{
				if (array4[num] != array6[num2])
				{
					throw new CryptographicException("Bad wrapped key size.");
				}
				num++;
				num2++;
			}
			return array5;
		}
		finally
		{
			cryptoTransform2?.Dispose();
			cryptoTransform?.Dispose();
			tripleDES?.Dispose();
		}
	}

	internal static byte[] AESKeyWrapEncrypt(byte[] rgbKey, byte[] rgbWrappedKeyData)
	{
		int num = rgbWrappedKeyData.Length >> 3;
		if (rgbWrappedKeyData.Length % 8 != 0 || num <= 0)
		{
			throw new CryptographicException("The length of the encrypted data in Key Wrap is either 32, 40 or 48 bytes.");
		}
		Aes aes = null;
		ICryptoTransform cryptoTransform = null;
		try
		{
			aes = Aes.Create();
			aes.Key = rgbKey;
			aes.Mode = CipherMode.ECB;
			aes.Padding = PaddingMode.None;
			cryptoTransform = aes.CreateEncryptor();
			if (num == 1)
			{
				byte[] array = new byte[s_rgbAES_KW_IV.Length + rgbWrappedKeyData.Length];
				Buffer.BlockCopy(s_rgbAES_KW_IV, 0, array, 0, s_rgbAES_KW_IV.Length);
				Buffer.BlockCopy(rgbWrappedKeyData, 0, array, s_rgbAES_KW_IV.Length, rgbWrappedKeyData.Length);
				return cryptoTransform.TransformFinalBlock(array, 0, array.Length);
			}
			long num2 = 0L;
			byte[] array2 = new byte[num + 1 << 3];
			Buffer.BlockCopy(rgbWrappedKeyData, 0, array2, 8, rgbWrappedKeyData.Length);
			byte[] array3 = new byte[8];
			byte[] array4 = new byte[16];
			Buffer.BlockCopy(s_rgbAES_KW_IV, 0, array3, 0, 8);
			for (int i = 0; i <= 5; i++)
			{
				for (int j = 1; j <= num; j++)
				{
					num2 = j + i * num;
					Buffer.BlockCopy(array3, 0, array4, 0, 8);
					Buffer.BlockCopy(array2, 8 * j, array4, 8, 8);
					byte[] array5 = cryptoTransform.TransformFinalBlock(array4, 0, 16);
					for (int k = 0; k < 8; k++)
					{
						byte b = (byte)((num2 >> 8 * (7 - k)) & 0xFF);
						array3[k] = (byte)(b ^ array5[k]);
					}
					Buffer.BlockCopy(array5, 8, array2, 8 * j, 8);
				}
			}
			Buffer.BlockCopy(array3, 0, array2, 0, 8);
			return array2;
		}
		finally
		{
			cryptoTransform?.Dispose();
			aes?.Dispose();
		}
	}

	internal static byte[] AESKeyWrapDecrypt(byte[] rgbKey, byte[] rgbEncryptedWrappedKeyData)
	{
		int num = (rgbEncryptedWrappedKeyData.Length >> 3) - 1;
		if (rgbEncryptedWrappedKeyData.Length % 8 != 0 || num <= 0)
		{
			throw new CryptographicException("The length of the encrypted data in Key Wrap is either 32, 40 or 48 bytes.");
		}
		byte[] array = new byte[num << 3];
		Aes aes = null;
		ICryptoTransform cryptoTransform = null;
		try
		{
			aes = Aes.Create();
			aes.Key = rgbKey;
			aes.Mode = CipherMode.ECB;
			aes.Padding = PaddingMode.None;
			cryptoTransform = aes.CreateDecryptor();
			if (num == 1)
			{
				byte[] array2 = cryptoTransform.TransformFinalBlock(rgbEncryptedWrappedKeyData, 0, rgbEncryptedWrappedKeyData.Length);
				for (int i = 0; i < 8; i++)
				{
					if (array2[i] != s_rgbAES_KW_IV[i])
					{
						throw new CryptographicException("Bad wrapped key size.");
					}
				}
				Buffer.BlockCopy(array2, 8, array, 0, 8);
				return array;
			}
			long num2 = 0L;
			Buffer.BlockCopy(rgbEncryptedWrappedKeyData, 8, array, 0, array.Length);
			byte[] array3 = new byte[8];
			byte[] array4 = new byte[16];
			Buffer.BlockCopy(rgbEncryptedWrappedKeyData, 0, array3, 0, 8);
			for (int num3 = 5; num3 >= 0; num3--)
			{
				for (int num4 = num; num4 >= 1; num4--)
				{
					num2 = num4 + num3 * num;
					for (int j = 0; j < 8; j++)
					{
						byte b = (byte)((num2 >> 8 * (7 - j)) & 0xFF);
						array3[j] ^= b;
					}
					Buffer.BlockCopy(array3, 0, array4, 0, 8);
					Buffer.BlockCopy(array, 8 * (num4 - 1), array4, 8, 8);
					byte[] src = cryptoTransform.TransformFinalBlock(array4, 0, 16);
					Buffer.BlockCopy(src, 8, array, 8 * (num4 - 1), 8);
					Buffer.BlockCopy(src, 0, array3, 0, 8);
				}
			}
			for (int k = 0; k < 8; k++)
			{
				if (array3[k] != s_rgbAES_KW_IV[k])
				{
					throw new CryptographicException("Bad wrapped key size.");
				}
			}
			return array;
		}
		finally
		{
			cryptoTransform?.Dispose();
			aes?.Dispose();
		}
	}
}
