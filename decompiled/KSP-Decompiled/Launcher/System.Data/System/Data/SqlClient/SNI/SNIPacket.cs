using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace System.Data.SqlClient.SNI;

internal class SNIPacket : IDisposable, IEquatable<SNIPacket>
{
	private byte[] _data;

	private int _length;

	private int _capacity;

	private int _offset;

	private string _description;

	private SNIAsyncCallback _completionCallback;

	public string Description
	{
		get
		{
			return _description;
		}
		set
		{
			_description = value;
		}
	}

	public int DataLeft => _length - _offset;

	public int Length => _length;

	public bool IsInvalid => _data == null;

	public SNIPacket(SNIHandle handle)
	{
		_offset = 0;
	}

	public void Dispose()
	{
		_data = null;
		_length = 0;
		_capacity = 0;
	}

	public void SetCompletionCallback(SNIAsyncCallback completionCallback)
	{
		_completionCallback = completionCallback;
	}

	public void InvokeCompletionCallback(uint sniErrorCode)
	{
		_completionCallback(this, sniErrorCode);
	}

	public void Allocate(int capacity)
	{
		_capacity = capacity;
		_data = new byte[capacity];
	}

	public SNIPacket Clone()
	{
		SNIPacket sNIPacket = new SNIPacket(null);
		sNIPacket._data = new byte[_length];
		Buffer.BlockCopy(_data, 0, sNIPacket._data, 0, _length);
		sNIPacket._length = _length;
		return sNIPacket;
	}

	public void GetData(byte[] buffer, ref int dataSize)
	{
		Buffer.BlockCopy(_data, 0, buffer, 0, _length);
		dataSize = _length;
	}

	public void SetData(byte[] data, int length)
	{
		_data = data;
		_length = length;
		_capacity = length;
		_offset = 0;
	}

	public int TakeData(SNIPacket packet, int size)
	{
		int num = TakeData(packet._data, packet._length, size);
		packet._length += num;
		return num;
	}

	public void AppendData(byte[] data, int size)
	{
		Buffer.BlockCopy(data, 0, _data, _length, size);
		_length += size;
	}

	public void AppendPacket(SNIPacket packet)
	{
		Buffer.BlockCopy(packet._data, 0, _data, _length, packet._length);
		_length += packet._length;
	}

	public int TakeData(byte[] buffer, int dataOffset, int size)
	{
		if (_offset >= _length)
		{
			return 0;
		}
		if (_offset + size > _length)
		{
			size = _length - _offset;
		}
		Buffer.BlockCopy(_data, _offset, buffer, dataOffset, size);
		_offset += size;
		return size;
	}

	public void Release()
	{
		_length = 0;
		_capacity = 0;
		_data = null;
	}

	public void Reset()
	{
		_length = 0;
		_data = new byte[_capacity];
	}

	public void ReadFromStreamAsync(Stream stream, SNIAsyncCallback callback)
	{
		bool error = false;
		stream.ReadAsync(_data, 0, _capacity).ContinueWith(delegate(Task<int> t)
		{
			Exception ex = ((t.Exception != null) ? t.Exception.InnerException : null);
			if (ex != null)
			{
				SNILoadHandle.SingletonInstance.LastError = new SNIError(SNIProviders.TCP_PROV, 35u, ex);
				error = true;
			}
			else
			{
				_length = t.Result;
				if (_length == 0)
				{
					SNILoadHandle.SingletonInstance.LastError = new SNIError(SNIProviders.TCP_PROV, 0u, 2u, string.Empty);
					error = true;
				}
			}
			if (error)
			{
				Release();
			}
			callback(this, error ? 1u : 0u);
		}, CancellationToken.None, TaskContinuationOptions.LongRunning | TaskContinuationOptions.DenyChildAttach, TaskScheduler.Default);
	}

	public void ReadFromStream(Stream stream)
	{
		_length = stream.Read(_data, 0, _capacity);
	}

	public void WriteToStream(Stream stream)
	{
		stream.Write(_data, 0, _length);
	}

	public override int GetHashCode()
	{
		return base.GetHashCode();
	}

	public override bool Equals(object obj)
	{
		if (obj is SNIPacket packet)
		{
			return Equals(packet);
		}
		return false;
	}

	public bool Equals(SNIPacket packet)
	{
		if (packet != null)
		{
			return packet == this;
		}
		return false;
	}
}
