using System;
using System.IO;
using System.Security.Cryptography;

namespace Mono.Security.Cryptography;

internal static class ManagedProtection
{
	private static RSA user;

	private static RSA machine;

	private static readonly object user_lock = new object();

	private static readonly object machine_lock = new object();

	public static byte[] Protect(byte[] userData, byte[] optionalEntropy, DataProtectionScope scope)
	{
		if (userData == null)
		{
			throw new ArgumentNullException("userData");
		}
		Rijndael rijndael = Rijndael.Create();
		rijndael.KeySize = 128;
		byte[] array = null;
		using (MemoryStream memoryStream = new MemoryStream())
		{
			ICryptoTransform transform = rijndael.CreateEncryptor();
			using CryptoStream cryptoStream = new CryptoStream(memoryStream, transform, CryptoStreamMode.Write);
			cryptoStream.Write(userData, 0, userData.Length);
			cryptoStream.Close();
			array = memoryStream.ToArray();
		}
		byte[] array2 = null;
		byte[] array3 = null;
		byte[] array4 = null;
		byte[] array5 = null;
		SHA256 sHA = SHA256.Create();
		try
		{
			array2 = rijndael.Key;
			array3 = rijndael.IV;
			array4 = new byte[68];
			byte[] src = sHA.ComputeHash(userData);
			if (optionalEntropy != null && optionalEntropy.Length != 0)
			{
				byte[] array6 = sHA.ComputeHash(optionalEntropy);
				for (int i = 0; i < 16; i++)
				{
					array2[i] ^= array6[i];
					array3[i] ^= array6[i + 16];
				}
				array4[0] = 2;
			}
			else
			{
				array4[0] = 1;
			}
			array4[1] = 16;
			Buffer.BlockCopy(array2, 0, array4, 2, 16);
			array4[18] = 16;
			Buffer.BlockCopy(array3, 0, array4, 19, 16);
			array4[35] = 32;
			Buffer.BlockCopy(src, 0, array4, 36, 32);
			array5 = new RSAOAEPKeyExchangeFormatter(GetKey(scope)).CreateKeyExchange(array4);
		}
		finally
		{
			if (array2 != null)
			{
				Array.Clear(array2, 0, array2.Length);
				array2 = null;
			}
			if (array4 != null)
			{
				Array.Clear(array4, 0, array4.Length);
				array4 = null;
			}
			if (array3 != null)
			{
				Array.Clear(array3, 0, array3.Length);
				array3 = null;
			}
			rijndael.Clear();
			sHA.Clear();
		}
		byte[] array7 = new byte[array5.Length + array.Length];
		Buffer.BlockCopy(array5, 0, array7, 0, array5.Length);
		Buffer.BlockCopy(array, 0, array7, array5.Length, array.Length);
		return array7;
	}

	public static byte[] Unprotect(byte[] encryptedData, byte[] optionalEntropy, DataProtectionScope scope)
	{
		if (encryptedData == null)
		{
			throw new ArgumentNullException("encryptedData");
		}
		byte[] array = null;
		Rijndael rijndael = Rijndael.Create();
		RSA key = GetKey(scope);
		int num = key.KeySize >> 3;
		bool flag = encryptedData.Length >= num;
		if (!flag)
		{
			num = encryptedData.Length;
		}
		byte[] array2 = new byte[num];
		Buffer.BlockCopy(encryptedData, 0, array2, 0, num);
		byte[] array3 = null;
		byte[] array4 = null;
		byte[] array5 = null;
		bool flag2 = false;
		bool flag3 = false;
		bool flag4 = false;
		SHA256 sHA = SHA256.Create();
		try
		{
			try
			{
				array3 = new RSAOAEPKeyExchangeDeformatter(key).DecryptKeyExchange(array2);
				flag2 = array3.Length == 68;
			}
			catch
			{
				flag2 = false;
			}
			if (!flag2)
			{
				array3 = new byte[68];
			}
			flag3 = array3[1] == 16 && array3[18] == 16 && array3[35] == 32;
			array4 = new byte[16];
			Buffer.BlockCopy(array3, 2, array4, 0, 16);
			array5 = new byte[16];
			Buffer.BlockCopy(array3, 19, array5, 0, 16);
			if (optionalEntropy != null && optionalEntropy.Length != 0)
			{
				byte[] array6 = sHA.ComputeHash(optionalEntropy);
				for (int i = 0; i < 16; i++)
				{
					array4[i] ^= array6[i];
					array5[i] ^= array6[i + 16];
				}
				flag3 &= array3[0] == 2;
			}
			else
			{
				flag3 &= array3[0] == 1;
			}
			using (MemoryStream memoryStream = new MemoryStream())
			{
				ICryptoTransform transform = rijndael.CreateDecryptor(array4, array5);
				using (CryptoStream cryptoStream = new CryptoStream(memoryStream, transform, CryptoStreamMode.Write))
				{
					try
					{
						cryptoStream.Write(encryptedData, num, encryptedData.Length - num);
						cryptoStream.Close();
					}
					catch
					{
					}
				}
				array = memoryStream.ToArray();
			}
			byte[] array7 = sHA.ComputeHash(array);
			flag4 = true;
			for (int j = 0; j < 32; j++)
			{
				if (array7[j] != array3[36 + j])
				{
					flag4 = false;
				}
			}
		}
		finally
		{
			if (array4 != null)
			{
				Array.Clear(array4, 0, array4.Length);
				array4 = null;
			}
			if (array3 != null)
			{
				Array.Clear(array3, 0, array3.Length);
				array3 = null;
			}
			if (array5 != null)
			{
				Array.Clear(array5, 0, array5.Length);
				array5 = null;
			}
			rijndael.Clear();
			sHA.Clear();
		}
		if (!flag || !flag2 || !flag3 || !flag4)
		{
			if (array != null)
			{
				Array.Clear(array, 0, array.Length);
				array = null;
			}
			throw new CryptographicException(global::Locale.GetText("Invalid data."));
		}
		return array;
	}

	private static RSA GetKey(DataProtectionScope scope)
	{
		switch (scope)
		{
		case DataProtectionScope.CurrentUser:
			if (user == null)
			{
				lock (user_lock)
				{
					CspParameters cspParameters2 = new CspParameters();
					cspParameters2.KeyContainerName = "DAPI";
					user = new RSACryptoServiceProvider(1536, cspParameters2);
				}
			}
			return user;
		case DataProtectionScope.LocalMachine:
			if (machine == null)
			{
				lock (machine_lock)
				{
					CspParameters cspParameters = new CspParameters();
					cspParameters.KeyContainerName = "DAPI";
					cspParameters.Flags = CspProviderFlags.UseMachineKeyStore;
					machine = new RSACryptoServiceProvider(1536, cspParameters);
				}
			}
			return machine;
		default:
			throw new CryptographicException(global::Locale.GetText("Invalid scope."));
		}
	}
}
