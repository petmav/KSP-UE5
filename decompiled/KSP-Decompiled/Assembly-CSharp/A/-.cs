using System;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Security;
using System.Security.Cryptography;

namespace A
{
	internal class _0003
	{
		private static readonly Assembly _001E;

		static _0003()
		{
			AppDomain.CurrentDomain.ResourceResolve += _0016;
			AppDomain.CurrentDomain.AssemblyResolve += _000D;
			Assembly executingAssembly = Assembly.GetExecutingAssembly();
			string assemblyString = _001B(executingAssembly);
			_001E = Assembly.Load(assemblyString);
		}

		private static Assembly _000D(object _001E, ResolveEventArgs _0003)
		{
			Assembly executingAssembly = Assembly.GetExecutingAssembly();
			string text = _001B(executingAssembly);
			if (_0003.Name.StartsWith(text))
			{
				Stream manifestResourceStream = executingAssembly.GetManifestResourceStream(text);
				byte[] rawAssembly = A._001E._000E(97L, manifestResourceStream);
				return Assembly.Load(rawAssembly);
			}
			return null;
		}

		private static string _001B(Assembly _001E)
		{
			string text = _001E.FullName;
			int num = text.IndexOf(',');
			if (num >= 0)
			{
				text = text.Substring(0, num);
			}
			return text + '&';
		}

		private static Assembly _0016(object _001E, ResolveEventArgs _0003)
		{
			if ((object)A._0003._001E != null)
			{
				string[] manifestResourceNames = A._0003._001E.GetManifestResourceNames();
				int num = 0;
				while (true)
				{
					if (num < manifestResourceNames.Length)
					{
						string text = manifestResourceNames[num];
						if (text == _0003.Name)
						{
							break;
						}
						num++;
						continue;
					}
					return null;
				}
				return A._0003._001E;
			}
			return A._0003._001E;
		}
	}
	internal class _001E
	{
		private static readonly object m__001E;

		private static readonly int _0003;

		private static readonly int _000D;

		private static readonly MemoryStream _001B;

		private static readonly MemoryStream _0016;

		private static readonly byte _0018;

		static _001E()
		{
			_001B = null;
			_0016 = null;
			_0003 = int.MaxValue;
			_000D = int.MinValue;
			_001B = new MemoryStream(0);
			_0016 = new MemoryStream(0);
			m__001E = new object();
		}

		private static string _0010(Assembly _001E)
		{
			string text = _001E.FullName;
			int num = text.IndexOf(',');
			if (num >= 0)
			{
				text = text.Substring(0, num);
			}
			return text;
		}

		private static byte[] _001F(Assembly _001E)
		{
			try
			{
				string fullName = _001E.FullName;
				int num = fullName.IndexOf("PublicKeyToken=");
				if (num < 0)
				{
					num = fullName.IndexOf("publickeytoken=");
				}
				if (num < 0)
				{
					return null;
				}
				num += 15;
				if (fullName[num] != 'n' && fullName[num] != 'N')
				{
					string s = fullName.Substring(num, 16);
					long value = long.Parse(s, NumberStyles.HexNumber);
					byte[] bytes = BitConverter.GetBytes(value);
					Array.Reverse((Array)bytes);
					return bytes;
				}
				return null;
			}
			catch
			{
			}
			return null;
		}

		internal static byte[] _0001(Stream _001E)
		{
			lock (A._001E.m__001E)
			{
				return _0015(97L, _001E);
			}
		}

		internal static byte[] _000E(long _001E, Stream _0003)
		{
			try
			{
				return _0001(_0003);
			}
			catch (HostProtectionException)
			{
				return _0015(97L, _0003);
			}
		}

		internal static byte[] _0015(long _001E, Stream _0003)
		{
			Stream stream = _0003;
			MemoryStream memoryStream = null;
			for (int i = 1; i < 4; i++)
			{
				_0003.ReadByte();
			}
			ushort num = (ushort)_0003.ReadByte();
			num = (ushort)(~num);
			if ((num & 2u) != 0)
			{
				DESCryptoServiceProvider dESCryptoServiceProvider = new DESCryptoServiceProvider();
				byte[] array = new byte[8];
				_0003.Read(array, 0, 8);
				dESCryptoServiceProvider.IV = array;
				byte[] array2 = new byte[8];
				_0003.Read(array2, 0, 8);
				bool flag = true;
				byte[] array3 = array2;
				for (int j = 0; j < array3.Length; j++)
				{
					if (array3[j] != 0)
					{
						flag = false;
						break;
					}
				}
				if (flag)
				{
					array2 = _001F(Assembly.GetExecutingAssembly());
				}
				dESCryptoServiceProvider.Key = array2;
				if (_001B == null)
				{
					if (A._001E._0003 == int.MaxValue)
					{
						_001B.Capacity = (int)_0003.Length;
					}
					else
					{
						_001B.Capacity = A._001E._0003;
					}
				}
				_001B.Position = 0L;
				ICryptoTransform cryptoTransform = dESCryptoServiceProvider.CreateDecryptor();
				int inputBlockSize = cryptoTransform.InputBlockSize;
				_ = cryptoTransform.OutputBlockSize;
				byte[] array4 = new byte[cryptoTransform.OutputBlockSize];
				byte[] array5 = new byte[cryptoTransform.InputBlockSize];
				int k;
				for (k = (int)_0003.Position; k + inputBlockSize < _0003.Length; k += inputBlockSize)
				{
					_0003.Read(array5, 0, inputBlockSize);
					int count = cryptoTransform.TransformBlock(array5, 0, inputBlockSize, array4, 0);
					_001B.Write(array4, 0, count);
				}
				_0003.Read(array5, 0, (int)(_0003.Length - k));
				byte[] array6 = cryptoTransform.TransformFinalBlock(array5, 0, (int)(_0003.Length - k));
				_001B.Write(array6, 0, array6.Length);
				stream = _001B;
				stream.Position = 0L;
				memoryStream = _001B;
			}
			if ((num & 8u) != 0)
			{
				if (_0016 == null)
				{
					if (_000D == int.MinValue)
					{
						_0016.Capacity = (int)stream.Length * 2;
					}
					else
					{
						_0016.Capacity = _000D;
					}
				}
				_0016.Position = 0L;
				DeflateStream deflateStream = new DeflateStream(stream, CompressionMode.Decompress);
				int num2 = 1000;
				byte[] buffer = new byte[1000];
				int num3;
				do
				{
					num3 = deflateStream.Read(buffer, 0, num2);
					if (num3 > 0)
					{
						_0016.Write(buffer, 0, num3);
					}
				}
				while (num3 >= num2);
				memoryStream = _0016;
			}
			if (memoryStream != null)
			{
				return memoryStream.ToArray();
			}
			byte[] array7 = new byte[_0003.Length - _0003.Position];
			_0003.Read(array7, 0, array7.Length);
			return array7;
		}
	}
}
