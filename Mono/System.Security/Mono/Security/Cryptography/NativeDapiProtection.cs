using System;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Cryptography;

namespace Mono.Security.Cryptography;

internal class NativeDapiProtection
{
	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
	private struct DATA_BLOB
	{
		private int cbData;

		private IntPtr pbData;

		public void Alloc(int size)
		{
			if (size > 0)
			{
				pbData = Marshal.AllocHGlobal(size);
				cbData = size;
			}
		}

		public void Alloc(byte[] managedMemory)
		{
			if (managedMemory != null)
			{
				int cb = managedMemory.Length;
				pbData = Marshal.AllocHGlobal(cb);
				cbData = cb;
				Marshal.Copy(managedMemory, 0, pbData, cbData);
			}
		}

		public void Free()
		{
			if (pbData != IntPtr.Zero)
			{
				ZeroMemory(pbData, cbData);
				Marshal.FreeHGlobal(pbData);
				pbData = IntPtr.Zero;
				cbData = 0;
			}
		}

		public byte[] ToBytes()
		{
			if (cbData <= 0)
			{
				return new byte[0];
			}
			byte[] array = new byte[cbData];
			Marshal.Copy(pbData, array, 0, cbData);
			return array;
		}
	}

	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
	private struct CRYPTPROTECT_PROMPTSTRUCT
	{
		private int cbSize;

		private uint dwPromptFlags;

		private IntPtr hwndApp;

		private string szPrompt;

		public CRYPTPROTECT_PROMPTSTRUCT(uint flags)
		{
			cbSize = Marshal.SizeOf(typeof(CRYPTPROTECT_PROMPTSTRUCT));
			dwPromptFlags = flags;
			hwndApp = IntPtr.Zero;
			szPrompt = null;
		}
	}

	private const uint CRYPTPROTECT_UI_FORBIDDEN = 1u;

	private const uint CRYPTPROTECT_LOCAL_MACHINE = 4u;

	[DllImport("crypt32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Auto, SetLastError = true)]
	[SuppressUnmanagedCodeSecurity]
	private static extern bool CryptProtectData(ref DATA_BLOB pDataIn, string szDataDescr, ref DATA_BLOB pOptionalEntropy, IntPtr pvReserved, ref CRYPTPROTECT_PROMPTSTRUCT pPromptStruct, uint dwFlags, ref DATA_BLOB pDataOut);

	[DllImport("crypt32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Auto, SetLastError = true)]
	[SuppressUnmanagedCodeSecurity]
	private static extern bool CryptUnprotectData(ref DATA_BLOB pDataIn, string szDataDescr, ref DATA_BLOB pOptionalEntropy, IntPtr pvReserved, ref CRYPTPROTECT_PROMPTSTRUCT pPromptStruct, uint dwFlags, ref DATA_BLOB pDataOut);

	[DllImport("kernel32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Auto, EntryPoint = "RtlZeroMemory")]
	[SuppressUnmanagedCodeSecurity]
	private static extern void ZeroMemory(IntPtr dest, int size);

	public static byte[] Protect(byte[] userData, byte[] optionalEntropy, DataProtectionScope scope)
	{
		byte[] array = null;
		int num = 0;
		DATA_BLOB pDataIn = default(DATA_BLOB);
		DATA_BLOB pOptionalEntropy = default(DATA_BLOB);
		DATA_BLOB pDataOut = default(DATA_BLOB);
		try
		{
			CRYPTPROTECT_PROMPTSTRUCT pPromptStruct = new CRYPTPROTECT_PROMPTSTRUCT(0u);
			pDataIn.Alloc(userData);
			pOptionalEntropy.Alloc(optionalEntropy);
			uint num2 = 1u;
			if (scope == DataProtectionScope.LocalMachine)
			{
				num2 |= 4u;
			}
			if (CryptProtectData(ref pDataIn, string.Empty, ref pOptionalEntropy, IntPtr.Zero, ref pPromptStruct, num2, ref pDataOut))
			{
				array = pDataOut.ToBytes();
			}
			else
			{
				num = Marshal.GetLastWin32Error();
			}
		}
		catch (Exception inner)
		{
			throw new CryptographicException(global::Locale.GetText("Error protecting data."), inner);
		}
		finally
		{
			pDataOut.Free();
			pDataIn.Free();
			pOptionalEntropy.Free();
		}
		if (array == null || num != 0)
		{
			throw new CryptographicException(num);
		}
		return array;
	}

	public static byte[] Unprotect(byte[] encryptedData, byte[] optionalEntropy, DataProtectionScope scope)
	{
		byte[] array = null;
		int num = 0;
		DATA_BLOB pDataIn = default(DATA_BLOB);
		DATA_BLOB pOptionalEntropy = default(DATA_BLOB);
		DATA_BLOB pDataOut = default(DATA_BLOB);
		try
		{
			CRYPTPROTECT_PROMPTSTRUCT pPromptStruct = new CRYPTPROTECT_PROMPTSTRUCT(0u);
			pDataIn.Alloc(encryptedData);
			pOptionalEntropy.Alloc(optionalEntropy);
			uint num2 = 1u;
			if (scope == DataProtectionScope.LocalMachine)
			{
				num2 |= 4u;
			}
			if (CryptUnprotectData(ref pDataIn, null, ref pOptionalEntropy, IntPtr.Zero, ref pPromptStruct, num2, ref pDataOut))
			{
				array = pDataOut.ToBytes();
			}
			else
			{
				num = Marshal.GetLastWin32Error();
			}
		}
		catch (Exception inner)
		{
			throw new CryptographicException(global::Locale.GetText("Error protecting data."), inner);
		}
		finally
		{
			pDataIn.Free();
			pDataOut.Free();
			pOptionalEntropy.Free();
		}
		if (array == null || num != 0)
		{
			throw new CryptographicException(num);
		}
		return array;
	}
}
