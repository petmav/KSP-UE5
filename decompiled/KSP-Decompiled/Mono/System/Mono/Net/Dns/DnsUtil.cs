using System;
using System.Text;

namespace Mono.Net.Dns;

internal static class DnsUtil
{
	public static bool IsValidDnsName(string name)
	{
		if (name == null)
		{
			return false;
		}
		int length = name.Length;
		if (length > 255)
		{
			return false;
		}
		int num = 0;
		for (int i = 0; i < length; i++)
		{
			if (name[i] == '.')
			{
				if (i == 0 && length > 1)
				{
					return false;
				}
				if (i > 0 && num == 0)
				{
					return false;
				}
				num = 0;
			}
			else
			{
				num++;
				if (num > 63)
				{
					return false;
				}
			}
		}
		return true;
	}

	public static int GetEncodedLength(string name)
	{
		if (!IsValidDnsName(name))
		{
			return -1;
		}
		if (name == string.Empty)
		{
			return 1;
		}
		int length = name.Length;
		if (name[length - 1] == '.')
		{
			return length + 1;
		}
		return length + 2;
	}

	public static int GetNameLength(byte[] buffer)
	{
		return GetNameLength(buffer, 0);
	}

	public static int GetNameLength(byte[] buffer, int offset)
	{
		if (buffer == null)
		{
			throw new ArgumentNullException("buffer");
		}
		if (offset < 0 || offset >= buffer.Length)
		{
			throw new ArgumentOutOfRangeException("offset");
		}
		int num = 0;
		int num2 = 0;
		while (num2 < 256)
		{
			num = buffer[offset++];
			if (num == 0)
			{
				if (num2 <= 0)
				{
					return 0;
				}
				return --num2;
			}
			int num3 = num & 0xC0;
			if (num3 == 192)
			{
				num = ((num3 & 0x3F) << 8) + buffer[offset++];
				offset = num;
				continue;
			}
			if (num3 >= 64)
			{
				return -2;
			}
			num2 += num + 1;
			offset += num;
		}
		return -1;
	}

	public static string ReadName(byte[] buffer, ref int offset)
	{
		if (buffer == null)
		{
			throw new ArgumentNullException("buffer");
		}
		if (offset < 0 || offset >= buffer.Length)
		{
			throw new ArgumentOutOfRangeException("offset");
		}
		StringBuilder stringBuilder = new StringBuilder(32);
		int num = 0;
		bool flag = true;
		int num2 = offset;
		while (stringBuilder.Length < 256)
		{
			num = buffer[num2++];
			if (flag)
			{
				offset++;
			}
			if (num == 0)
			{
				if (stringBuilder.Length > 0)
				{
					stringBuilder.Length--;
				}
				return stringBuilder.ToString();
			}
			int num3 = num & 0xC0;
			if (num3 == 192)
			{
				num = ((num3 & 0x3F) << 8) + buffer[num2];
				if (flag)
				{
					offset++;
				}
				flag = false;
				num2 = num;
				continue;
			}
			if (num >= 64)
			{
				return null;
			}
			for (int i = 0; i < num; i++)
			{
				stringBuilder.Append((char)buffer[num2 + i]);
			}
			stringBuilder.Append('.');
			num2 += num;
			if (flag)
			{
				offset += num;
			}
		}
		return null;
	}
}
