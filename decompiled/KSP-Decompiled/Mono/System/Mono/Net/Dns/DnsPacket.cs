using System;

namespace Mono.Net.Dns;

internal abstract class DnsPacket
{
	protected byte[] packet;

	protected int position;

	protected DnsHeader header;

	public byte[] Packet => packet;

	public int Length => position;

	public DnsHeader Header => header;

	protected DnsPacket()
	{
	}

	protected DnsPacket(int length)
		: this(new byte[length], length)
	{
	}

	protected DnsPacket(byte[] buffer, int length)
	{
		if (buffer == null)
		{
			throw new ArgumentNullException("buffer");
		}
		if (length <= 0)
		{
			throw new ArgumentOutOfRangeException("length", "Must be greater than zero.");
		}
		packet = buffer;
		position = length;
		header = new DnsHeader(new ArraySegment<byte>(packet, 0, 12));
	}

	protected void WriteUInt16(ushort v)
	{
		packet[position++] = (byte)((v & 0xFF00) >> 8);
		packet[position++] = (byte)(v & 0xFFu);
	}

	protected void WriteStringBytes(string str, int offset, int count)
	{
		int num = offset;
		int num2 = 0;
		while (num2 < count)
		{
			packet[position++] = (byte)str[num];
			num2++;
			num++;
		}
	}

	protected void WriteLabel(string str, int offset, int count)
	{
		packet[position++] = (byte)count;
		WriteStringBytes(str, offset, count);
	}

	protected void WriteDnsName(string name)
	{
		if (!DnsUtil.IsValidDnsName(name))
		{
			throw new ArgumentException("Invalid DNS name");
		}
		if (!string.IsNullOrEmpty(name))
		{
			int length = name.Length;
			int num = 0;
			int num2 = 0;
			for (int i = 0; i < length; i++)
			{
				if (name[i] != '.')
				{
					num2++;
					continue;
				}
				if (i == 0)
				{
					break;
				}
				WriteLabel(name, num, num2);
				num += num2 + 1;
				num2 = 0;
			}
			if (num2 > 0)
			{
				WriteLabel(name, num, num2);
			}
		}
		packet[position++] = 0;
	}

	protected internal string ReadName(ref int offset)
	{
		return DnsUtil.ReadName(packet, ref offset);
	}

	protected internal static string ReadName(byte[] buffer, ref int offset)
	{
		return DnsUtil.ReadName(buffer, ref offset);
	}

	protected internal ushort ReadUInt16(ref int offset)
	{
		return (ushort)((packet[offset++] << 8) + packet[offset++]);
	}

	protected internal int ReadInt32(ref int offset)
	{
		return (packet[offset++] << 24) + (packet[offset++] << 16) + (packet[offset++] << 8) + packet[offset++];
	}
}
