using System;
using System.IO;

namespace Mono.Audio;

internal class AuData : AudioData
{
	private Stream stream;

	private short channels;

	private ushort frame_divider;

	private int sample_rate;

	private int data_len;

	private AudioFormat format;

	public override int Channels => channels;

	public override int Rate => sample_rate;

	public override AudioFormat Format => format;

	public AuData(Stream data)
	{
		stream = data;
		byte[] array = new byte[24];
		int num = stream.Read(array, 0, 24);
		if (num != 24 || array[0] != 46 || array[1] != 115 || array[2] != 110 || array[3] != 100)
		{
			throw new Exception("incorrect format" + num);
		}
		int num2 = array[7];
		num2 |= array[6] << 8;
		num2 |= array[5] << 16;
		num2 |= array[4] << 24;
		data_len = array[11];
		data_len |= array[10] << 8;
		data_len |= array[9] << 16;
		data_len |= array[8] << 24;
		int num3 = array[15];
		num3 |= array[14] << 8;
		num3 |= array[13] << 16;
		num3 |= array[12] << 24;
		sample_rate = array[19];
		sample_rate |= array[18] << 8;
		sample_rate |= array[17] << 16;
		sample_rate |= array[16] << 24;
		int num4 = array[23];
		num4 |= array[22] << 8;
		num4 |= array[21] << 16;
		num4 |= array[20] << 24;
		channels = (short)num4;
		if (num2 < 24 || (num4 != 1 && num4 != 2))
		{
			throw new Exception("incorrect format offset" + num2);
		}
		if (num2 != 24)
		{
			for (int i = 24; i < num2; i++)
			{
				stream.ReadByte();
			}
		}
		if (num3 == 1)
		{
			frame_divider = 1;
			format = AudioFormat.MU_LAW;
			if (data_len == -1)
			{
				data_len = (int)stream.Length - num2;
			}
			return;
		}
		throw new Exception("incorrect format encoding" + num3);
	}

	public override void Play(AudioDevice dev)
	{
		int num = 0;
		int num2 = 0;
		int chunkSize = (int)dev.ChunkSize;
		int num3 = data_len;
		byte[] array = new byte[data_len];
		byte[] array2 = new byte[chunkSize];
		stream.Position = 0L;
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
