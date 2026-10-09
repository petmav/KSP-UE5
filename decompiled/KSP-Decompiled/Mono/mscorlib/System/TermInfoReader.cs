using System.IO;
using System.Text;

namespace System;

internal class TermInfoReader
{
	private short boolSize;

	private short numSize;

	private short strOffsets;

	private byte[] buffer;

	private int booleansOffset;

	public TermInfoReader(string term, string filename)
	{
		using FileStream fileStream = File.OpenRead(filename);
		long length = fileStream.Length;
		if (length > 4096)
		{
			throw new Exception("File must be smaller than 4K");
		}
		buffer = new byte[(int)length];
		if (fileStream.Read(buffer, 0, buffer.Length) != buffer.Length)
		{
			throw new Exception("Short read");
		}
		ReadHeader(buffer, ref booleansOffset);
		ReadNames(buffer, ref booleansOffset);
	}

	public TermInfoReader(string term, byte[] buffer)
	{
		if (buffer == null)
		{
			throw new ArgumentNullException("buffer");
		}
		this.buffer = buffer;
		ReadHeader(buffer, ref booleansOffset);
		ReadNames(buffer, ref booleansOffset);
	}

	private void ReadHeader(byte[] buffer, ref int position)
	{
		short @int = GetInt16(buffer, position);
		position += 2;
		if (@int != 282)
		{
			throw new Exception($"Magic number is wrong: {@int}");
		}
		GetInt16(buffer, position);
		position += 2;
		boolSize = GetInt16(buffer, position);
		position += 2;
		numSize = GetInt16(buffer, position);
		position += 2;
		strOffsets = GetInt16(buffer, position);
		position += 2;
		GetInt16(buffer, position);
		position += 2;
	}

	private void ReadNames(byte[] buffer, ref int position)
	{
		string @string = GetString(buffer, position);
		position += @string.Length + 1;
	}

	public bool Get(TermInfoBooleans boolean)
	{
		if (boolean < TermInfoBooleans.AutoLeftMargin || boolean >= TermInfoBooleans.Last || (int)boolean >= (int)boolSize)
		{
			return false;
		}
		int num = booleansOffset;
		num = (int)(num + boolean);
		return buffer[num] != 0;
	}

	public int Get(TermInfoNumbers number)
	{
		if (number < TermInfoNumbers.Columns || number >= TermInfoNumbers.Last || (int)number > (int)numSize)
		{
			return -1;
		}
		int num = booleansOffset + boolSize;
		if (num % 2 == 1)
		{
			num++;
		}
		num += (int)number * 2;
		return GetInt16(buffer, num);
	}

	public string Get(TermInfoStrings tstr)
	{
		if (tstr < TermInfoStrings.BackTab || tstr >= TermInfoStrings.Last || (int)tstr > (int)strOffsets)
		{
			return null;
		}
		int num = booleansOffset + boolSize;
		if (num % 2 == 1)
		{
			num++;
		}
		num += numSize * 2;
		int @int = GetInt16(buffer, num + (int)tstr * 2);
		if (@int == -1)
		{
			return null;
		}
		return GetString(buffer, num + strOffsets * 2 + @int);
	}

	public byte[] GetStringBytes(TermInfoStrings tstr)
	{
		if (tstr < TermInfoStrings.BackTab || tstr >= TermInfoStrings.Last || (int)tstr > (int)strOffsets)
		{
			return null;
		}
		int num = booleansOffset + boolSize;
		if (num % 2 == 1)
		{
			num++;
		}
		num += numSize * 2;
		int @int = GetInt16(buffer, num + (int)tstr * 2);
		if (@int == -1)
		{
			return null;
		}
		return GetStringBytes(buffer, num + strOffsets * 2 + @int);
	}

	private short GetInt16(byte[] buffer, int offset)
	{
		int num = buffer[offset];
		int num2 = buffer[offset + 1];
		if (num == 255 && num2 == 255)
		{
			return -1;
		}
		return (short)(num + num2 * 256);
	}

	private string GetString(byte[] buffer, int offset)
	{
		int num = 0;
		int num2 = offset;
		while (buffer[num2++] != 0)
		{
			num++;
		}
		return Encoding.ASCII.GetString(buffer, offset, num);
	}

	private byte[] GetStringBytes(byte[] buffer, int offset)
	{
		int num = 0;
		int num2 = offset;
		while (buffer[num2++] != 0)
		{
			num++;
		}
		byte[] array = new byte[num];
		Buffer.InternalBlockCopy(buffer, offset, array, 0, num);
		return array;
	}

	internal static string Escape(string s)
	{
		StringBuilder stringBuilder = new StringBuilder();
		foreach (char c in s)
		{
			if (char.IsControl(c))
			{
				stringBuilder.AppendFormat("\\x{0:X2}", (int)c);
			}
			else
			{
				stringBuilder.Append(c);
			}
		}
		return stringBuilder.ToString();
	}
}
