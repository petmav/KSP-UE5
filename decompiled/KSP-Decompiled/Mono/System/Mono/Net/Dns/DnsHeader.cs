using System;
using System.Text;

namespace Mono.Net.Dns;

internal class DnsHeader
{
	public const int DnsHeaderLength = 12;

	private ArraySegment<byte> bytes;

	public ushort ID
	{
		get
		{
			return (ushort)(bytes.Array[bytes.Offset] * 256 + bytes.Array[bytes.Offset + 1]);
		}
		set
		{
			bytes.Array[bytes.Offset] = (byte)((value & 0xFF00) >> 8);
			bytes.Array[bytes.Offset + 1] = (byte)(value & 0xFFu);
		}
	}

	public bool IsQuery
	{
		get
		{
			return (bytes.Array[2 + bytes.Offset] & 0x80) != 0;
		}
		set
		{
			if (!value)
			{
				bytes.Array[2 + bytes.Offset] |= 128;
			}
			else
			{
				bytes.Array[2 + bytes.Offset] &= 127;
			}
		}
	}

	public DnsOpCode OpCode
	{
		get
		{
			return (DnsOpCode)((bytes.Array[2 + bytes.Offset] & 0x78) >> 3);
		}
		set
		{
			if (!Enum.IsDefined(typeof(DnsOpCode), value))
			{
				throw new ArgumentOutOfRangeException("value", "Invalid DnsOpCode value");
			}
			int num = (int)value;
			num <<= 3;
			int num2 = bytes.Array[2 + bytes.Offset] & 0x87;
			num |= num2;
			bytes.Array[2 + bytes.Offset] = (byte)num;
		}
	}

	public bool AuthoritativeAnswer
	{
		get
		{
			return (bytes.Array[2 + bytes.Offset] & 4) != 0;
		}
		set
		{
			if (value)
			{
				bytes.Array[2 + bytes.Offset] |= 4;
			}
			else
			{
				bytes.Array[2 + bytes.Offset] &= 251;
			}
		}
	}

	public bool Truncation
	{
		get
		{
			return (bytes.Array[2 + bytes.Offset] & 2) != 0;
		}
		set
		{
			if (value)
			{
				bytes.Array[2 + bytes.Offset] |= 2;
			}
			else
			{
				bytes.Array[2 + bytes.Offset] &= 253;
			}
		}
	}

	public bool RecursionDesired
	{
		get
		{
			return (bytes.Array[2 + bytes.Offset] & 1) != 0;
		}
		set
		{
			if (value)
			{
				bytes.Array[2 + bytes.Offset] |= 1;
			}
			else
			{
				bytes.Array[2 + bytes.Offset] &= 254;
			}
		}
	}

	public bool RecursionAvailable
	{
		get
		{
			return (bytes.Array[3 + bytes.Offset] & 0x80) != 0;
		}
		set
		{
			if (value)
			{
				bytes.Array[3 + bytes.Offset] |= 128;
			}
			else
			{
				bytes.Array[3 + bytes.Offset] &= 127;
			}
		}
	}

	public int ZReserved
	{
		get
		{
			return (bytes.Array[3 + bytes.Offset] & 0x70) >> 4;
		}
		set
		{
			if (value < 0 || value > 7)
			{
				throw new ArgumentOutOfRangeException("value", "Must be between 0 and 7");
			}
			bytes.Array[3 + bytes.Offset] &= 143;
			bytes.Array[3 + bytes.Offset] |= (byte)((value << 4) & 0x70);
		}
	}

	public DnsRCode RCode
	{
		get
		{
			return (DnsRCode)(bytes.Array[3 + bytes.Offset] & 0xFu);
		}
		set
		{
			if ((int)value < 0 || (int)value > 15)
			{
				throw new ArgumentOutOfRangeException("value", "Must be between 0 and 15");
			}
			bytes.Array[3 + bytes.Offset] &= 15;
			bytes.Array[3 + bytes.Offset] |= (byte)value;
		}
	}

	public ushort QuestionCount
	{
		get
		{
			return GetUInt16(bytes.Array, 4);
		}
		set
		{
			SetUInt16(bytes.Array, 4, value);
		}
	}

	public ushort AnswerCount
	{
		get
		{
			return GetUInt16(bytes.Array, 6);
		}
		set
		{
			SetUInt16(bytes.Array, 6, value);
		}
	}

	public ushort AuthorityCount
	{
		get
		{
			return GetUInt16(bytes.Array, 8);
		}
		set
		{
			SetUInt16(bytes.Array, 8, value);
		}
	}

	public ushort AdditionalCount
	{
		get
		{
			return GetUInt16(bytes.Array, 10);
		}
		set
		{
			SetUInt16(bytes.Array, 10, value);
		}
	}

	public DnsHeader(byte[] bytes)
		: this(bytes, 0)
	{
	}

	public DnsHeader(byte[] bytes, int offset)
		: this(new ArraySegment<byte>(bytes, offset, 12))
	{
	}

	public DnsHeader(ArraySegment<byte> segment)
	{
		if (segment.Count != 12)
		{
			throw new ArgumentException("Count must be 12", "segment");
		}
		bytes = segment;
	}

	public void Clear()
	{
		for (int i = 0; i < 12; i++)
		{
			bytes.Array[i + bytes.Offset] = 0;
		}
	}

	private static ushort GetUInt16(byte[] bytes, int offset)
	{
		return (ushort)(bytes[offset] * 256 + bytes[offset + 1]);
	}

	private static void SetUInt16(byte[] bytes, int offset, ushort val)
	{
		bytes[offset] = (byte)((val & 0xFF00) >> 8);
		bytes[offset + 1] = (byte)(val & 0xFFu);
	}

	public override string ToString()
	{
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.AppendFormat("ID: {0} QR: {1} Opcode: {2} AA: {3} TC: {4} RD: {5} RA: {6} \r\nRCode: {7} ", ID, IsQuery, OpCode, AuthoritativeAnswer, Truncation, RecursionDesired, RecursionAvailable, RCode);
		stringBuilder.AppendFormat("Q: {0} A: {1} NS: {2} AR: {3}\r\n", QuestionCount, AnswerCount, AuthorityCount, AdditionalCount);
		return stringBuilder.ToString();
	}
}
