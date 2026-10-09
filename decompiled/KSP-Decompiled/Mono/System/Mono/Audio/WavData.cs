using System;
using System.IO;

namespace Mono.Audio;

internal class WavData : AudioData
{
	private Stream stream;

	private short channels;

	private ushort frame_divider;

	private int sample_rate;

	private int data_len;

	private long data_offset;

	private AudioFormat format;

	public override int Channels => channels;

	public override int Rate => sample_rate;

	public override AudioFormat Format => format;

	public WavData(Stream data)
	{
		stream = data;
		byte[] array = new byte[44];
		int num = stream.Read(array, 0, 12);
		if (num != 12 || array[0] != 82 || array[1] != 73 || array[2] != 70 || array[3] != 70 || array[8] != 87 || array[9] != 65 || array[10] != 86 || array[11] != 69)
		{
			throw new Exception("incorrect format" + num);
		}
		num = stream.Read(array, 0, 8);
		if (num == 8 && array[0] == 102 && array[1] == 109 && array[2] == 116 && array[3] == 32)
		{
			int num2 = array[4];
			num2 |= array[5] << 8;
			num2 |= array[6] << 16;
			num2 |= array[7] << 24;
			num = stream.Read(array, 0, num2);
			if (num2 == num)
			{
				int num3 = 0;
				if ((array[num3++] | (array[num3++] << 8)) != 1)
				{
					throw new Exception("incorrect format (not PCM)");
				}
				channels = (short)(array[num3++] | (array[num3++] << 8));
				sample_rate = array[num3++];
				sample_rate |= array[num3++] << 8;
				sample_rate |= array[num3++] << 16;
				sample_rate |= array[num3++] << 24;
				_ = array[num3++] | (array[num3++] << 8) | (array[num3++] << 16);
				_ = array[num3++];
				num3 += 2;
				switch (array[num3++] | (array[num3++] << 8))
				{
				case 8:
					frame_divider = 1;
					format = AudioFormat.U8;
					break;
				case 16:
					frame_divider = 2;
					format = AudioFormat.S16_LE;
					break;
				default:
					throw new Exception("bits per sample");
				}
				num = stream.Read(array, 0, 8);
				if (num == 8)
				{
					if (array[0] == 102 && array[1] == 97 && array[2] == 99 && array[3] == 116)
					{
						int num4 = array[4];
						num4 |= array[5] << 8;
						num4 |= array[6] << 16;
						num4 |= array[7] << 24;
						num = stream.Read(array, 0, num4);
						num = stream.Read(array, 0, 8);
					}
					if (array[0] != 100 || array[1] != 97 || array[2] != 116 || array[3] != 97)
					{
						throw new Exception("incorrect format (data/fact chunck)");
					}
					int num5 = array[4];
					num5 |= array[5] << 8;
					num5 |= array[6] << 16;
					num5 |= array[7] << 24;
					data_len = num5;
					data_offset = stream.Position;
				}
				return;
			}
			throw new Exception("Error: Can't Read " + num2 + " bytes from stream (" + num + " bytes read");
		}
		throw new Exception("incorrect format (fmt)");
	}

	public override void Play(AudioDevice dev)
	{
		int num = 0;
		int num2 = 0;
		int chunkSize = (int)dev.ChunkSize;
		int num3 = data_len;
		byte[] array = new byte[data_len];
		byte[] array2 = new byte[chunkSize];
		stream.Position = data_offset;
		stream.Read(array, 0, data_len);
		while (!IsStopped && num3 >= 0)
		{
			Buffer.BlockCopy(array, num2, array2, 0, chunkSize);
			num = dev.PlaySample(array2, chunkSize / (frame_divider * channels));
			if (num > 0)
			{
				num2 += num * frame_divider * channels;
				num3 -= num * frame_divider * channels;
			}
		}
	}
}
