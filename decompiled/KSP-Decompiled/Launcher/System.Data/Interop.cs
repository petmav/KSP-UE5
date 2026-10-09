using System;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

internal static class Interop
{
	private static class Libraries
	{
		internal const string SystemNative = "System.Native";

		internal const string HttpNative = "System.Net.Http.Native";

		internal const string NetSecurityNative = "System.Net.Security.Native";

		internal const string CryptoNative = "System.Security.Cryptography.Native.OpenSsl";

		internal const string GlobalizationNative = "System.Globalization.Native";

		internal const string CompressionNative = "System.IO.Compression.Native";

		internal const string Libdl = "libdl";
	}

	internal static class NetSecurityNative
	{
		internal sealed class GssApiException : Exception
		{
			private readonly Status _minorStatus;

			public Status MinorStatus => _minorStatus;

			public GssApiException(string message)
				: base(message)
			{
			}

			public GssApiException(Status majorStatus, Status minorStatus)
				: base(GetGssApiDisplayStatus(majorStatus, minorStatus))
			{
				base.HResult = (int)majorStatus;
				_minorStatus = minorStatus;
			}

			private static string GetGssApiDisplayStatus(Status majorStatus, Status minorStatus)
			{
				string gssApiDisplayStatus = GetGssApiDisplayStatus(majorStatus, isMinor: false);
				string gssApiDisplayStatus2 = GetGssApiDisplayStatus(minorStatus, isMinor: true);
				if (gssApiDisplayStatus == null || gssApiDisplayStatus2 == null)
				{
					return global::SR.Format("GSSAPI operation failed with status: {0} (Minor status: {1}).", majorStatus.ToString("x"), minorStatus.ToString("x"));
				}
				return global::SR.Format("GSSAPI operation failed with error - {0} ({1}).", gssApiDisplayStatus, gssApiDisplayStatus2);
			}

			private static string GetGssApiDisplayStatus(Status status, bool isMinor)
			{
				GssBuffer buffer = default(GssBuffer);
				try
				{
					Status minorStatus;
					return ((isMinor ? DisplayMinorStatus(out minorStatus, status, ref buffer) : DisplayMajorStatus(out minorStatus, status, ref buffer)) != 0) ? null : Marshal.PtrToStringAnsi(buffer._data);
				}
				finally
				{
					buffer.Dispose();
				}
			}
		}

		internal struct GssBuffer : IDisposable
		{
			internal ulong _length;

			internal IntPtr _data;

			internal int Copy(byte[] destination, int offset)
			{
				if (_data == IntPtr.Zero || _length == 0L)
				{
					return 0;
				}
				int num = Convert.ToInt32(_length);
				int num2 = destination.Length - offset;
				if (num > num2)
				{
					throw new GssApiException(global::SR.Format("Insufficient buffer space. Required: {0} Actual: {1}.", num, num2));
				}
				Marshal.Copy(_data, destination, offset, num);
				return num;
			}

			internal byte[] ToByteArray()
			{
				if (_data == IntPtr.Zero || _length == 0L)
				{
					return Array.Empty<byte>();
				}
				int num = Convert.ToInt32(_length);
				byte[] array = new byte[num];
				Marshal.Copy(_data, array, 0, num);
				return array;
			}

			public void Dispose()
			{
				if (_data != IntPtr.Zero)
				{
					ReleaseGssBuffer(_data, _length);
					_data = IntPtr.Zero;
				}
				_length = 0uL;
			}
		}

		internal enum Status : uint
		{
			GSS_S_COMPLETE,
			GSS_S_CONTINUE_NEEDED
		}

		[Flags]
		internal enum GssFlags : uint
		{
			GSS_C_DELEG_FLAG = 1u,
			GSS_C_MUTUAL_FLAG = 2u,
			GSS_C_REPLAY_FLAG = 4u,
			GSS_C_SEQUENCE_FLAG = 8u,
			GSS_C_CONF_FLAG = 0x10u,
			GSS_C_INTEG_FLAG = 0x20u,
			GSS_C_ANON_FLAG = 0x40u,
			GSS_C_PROT_READY_FLAG = 0x80u,
			GSS_C_TRANS_FLAG = 0x100u,
			GSS_C_DCE_STYLE = 0x1000u,
			GSS_C_IDENTIFY_FLAG = 0x2000u,
			GSS_C_EXTENDED_ERROR_FLAG = 0x4000u,
			GSS_C_DELEG_POLICY_FLAG = 0x8000u
		}

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_ReleaseGssBuffer")]
		internal static extern void ReleaseGssBuffer(IntPtr bufferPtr, ulong length);

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_DisplayMinorStatus")]
		internal static extern Status DisplayMinorStatus(out Status minorStatus, Status statusValue, ref GssBuffer buffer);

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_DisplayMajorStatus")]
		internal static extern Status DisplayMajorStatus(out Status minorStatus, Status statusValue, ref GssBuffer buffer);

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_ImportUserName")]
		internal static extern Status ImportUserName(out Status minorStatus, string inputName, int inputNameByteCount, out SafeGssNameHandle outputName);

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_ImportPrincipalName")]
		internal static extern Status ImportPrincipalName(out Status minorStatus, string inputName, int inputNameByteCount, out SafeGssNameHandle outputName);

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_ReleaseName")]
		internal static extern Status ReleaseName(out Status minorStatus, ref IntPtr inputName);

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_InitiateCredSpNego")]
		internal static extern Status InitiateCredSpNego(out Status minorStatus, SafeGssNameHandle desiredName, out SafeGssCredHandle outputCredHandle);

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_InitiateCredWithPassword")]
		internal static extern Status InitiateCredWithPassword(out Status minorStatus, bool isNtlm, SafeGssNameHandle desiredName, string password, int passwordLen, out SafeGssCredHandle outputCredHandle);

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_ReleaseCred")]
		internal static extern Status ReleaseCred(out Status minorStatus, ref IntPtr credHandle);

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_InitSecContext")]
		internal static extern Status InitSecContext(out Status minorStatus, SafeGssCredHandle initiatorCredHandle, ref SafeGssContextHandle contextHandle, bool isNtlmOnly, SafeGssNameHandle targetName, uint reqFlags, byte[] inputBytes, int inputLength, ref GssBuffer token, out uint retFlags, out int isNtlmUsed);

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_AcceptSecContext")]
		internal static extern Status AcceptSecContext(out Status minorStatus, ref SafeGssContextHandle acceptContextHandle, byte[] inputBytes, int inputLength, ref GssBuffer token);

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_DeleteSecContext")]
		internal static extern Status DeleteSecContext(out Status minorStatus, ref IntPtr contextHandle);

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_Wrap")]
		private static extern Status Wrap(out Status minorStatus, SafeGssContextHandle contextHandle, bool isEncrypt, byte[] inputBytes, int offset, int count, ref GssBuffer outBuffer);

		[DllImport("System.Net.Security.Native", EntryPoint = "NetSecurityNative_Unwrap")]
		private static extern Status Unwrap(out Status minorStatus, SafeGssContextHandle contextHandle, byte[] inputBytes, int offset, int count, ref GssBuffer outBuffer);

		internal static Status WrapBuffer(out Status minorStatus, SafeGssContextHandle contextHandle, bool isEncrypt, byte[] inputBytes, int offset, int count, ref GssBuffer outBuffer)
		{
			return Wrap(out minorStatus, contextHandle, isEncrypt, inputBytes, offset, count, ref outBuffer);
		}

		internal static Status UnwrapBuffer(out Status minorStatus, SafeGssContextHandle contextHandle, byte[] inputBytes, int offset, int count, ref GssBuffer outBuffer)
		{
			return Unwrap(out minorStatus, contextHandle, inputBytes, offset, count, ref outBuffer);
		}
	}
}
