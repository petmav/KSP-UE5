using System.IO;
using System.IO.Pipes;

namespace System.Data.SqlClient.SNI;

internal sealed class SslOverTdsStream : Stream
{
	private readonly Stream _stream;

	private int _packetBytes;

	private bool _encapsulate;

	private const int PACKET_SIZE_WITHOUT_HEADER = 4088;

	private const int PRELOGIN_PACKET_TYPE = 18;

	public override long Position
	{
		get
		{
			throw new NotSupportedException();
		}
		set
		{
			throw new NotSupportedException();
		}
	}

	public override bool CanRead => _stream.CanRead;

	public override bool CanWrite => _stream.CanWrite;

	public override bool CanSeek => false;

	public override long Length
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public SslOverTdsStream(Stream stream)
	{
		_stream = stream;
		_encapsulate = true;
	}

	public void FinishHandshake()
	{
		_encapsulate = false;
	}

	public override int Read(byte[] buffer, int offset, int count)
	{
		int i = 0;
		byte[] array = new byte[(count < 8) ? 8 : count];
		if (_encapsulate)
		{
			if (_packetBytes == 0)
			{
				for (; i < 8; i += _stream.Read(array, i, 8 - i))
				{
				}
				_packetBytes = (array[2] << 8) | array[3];
				_packetBytes -= 8;
			}
			if (count > _packetBytes)
			{
				count = _packetBytes;
			}
		}
		i = _stream.Read(array, 0, count);
		if (_encapsulate)
		{
			_packetBytes -= i;
		}
		Buffer.BlockCopy(array, 0, buffer, offset, i);
		return i;
	}

	public override void Write(byte[] buffer, int offset, int count)
	{
		int num = 0;
		int num2 = offset;
		while (count > 0)
		{
			if (_encapsulate)
			{
				num = ((count <= 4088) ? count : 4088);
				count -= num;
				byte[] array = new byte[8 + num];
				array[0] = 18;
				array[1] = ((count <= 0) ? ((byte)1) : ((byte)0));
				array[2] = (byte)((num + 8) / 256);
				array[3] = (byte)((num + 8) % 256);
				array[4] = 0;
				array[5] = 0;
				array[6] = 0;
				array[7] = 0;
				for (int i = 8; i < array.Length; i++)
				{
					array[i] = buffer[num2 + (i - 8)];
				}
				_stream.Write(array, 0, array.Length);
			}
			else
			{
				num = count;
				count = 0;
				_stream.Write(buffer, num2, num);
			}
			_stream.Flush();
			num2 += num;
		}
	}

	public override void SetLength(long value)
	{
		throw new NotSupportedException();
	}

	public override void Flush()
	{
		if (!(_stream is PipeStream))
		{
			_stream.Flush();
		}
	}

	public override long Seek(long offset, SeekOrigin origin)
	{
		throw new NotSupportedException();
	}
}
