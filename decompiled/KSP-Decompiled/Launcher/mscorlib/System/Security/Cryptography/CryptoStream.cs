using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;

namespace System.Security.Cryptography;

[ComVisible(true)]
public class CryptoStream : Stream, IDisposable
{
	[StructLayout(LayoutKind.Sequential, Size = 1)]
	private struct HopToThreadPoolAwaitable : INotifyCompletion
	{
		public bool IsCompleted => false;

		public HopToThreadPoolAwaitable GetAwaiter()
		{
			return this;
		}

		public void OnCompleted(Action continuation)
		{
			Task.Run(continuation);
		}

		public void GetResult()
		{
		}
	}

	private Stream _stream;

	private ICryptoTransform _Transform;

	private byte[] _InputBuffer;

	private int _InputBufferIndex;

	private int _InputBlockSize;

	private byte[] _OutputBuffer;

	private int _OutputBufferIndex;

	private int _OutputBlockSize;

	private CryptoStreamMode _transformMode;

	private bool _canRead;

	private bool _canWrite;

	private bool _finalBlockTransformed;

	public override bool CanRead => _canRead;

	public override bool CanSeek => false;

	public override bool CanWrite => _canWrite;

	public override long Length
	{
		get
		{
			throw new NotSupportedException(Environment.GetResourceString("Stream does not support seeking."));
		}
	}

	public override long Position
	{
		get
		{
			throw new NotSupportedException(Environment.GetResourceString("Stream does not support seeking."));
		}
		set
		{
			throw new NotSupportedException(Environment.GetResourceString("Stream does not support seeking."));
		}
	}

	public bool HasFlushedFinalBlock => _finalBlockTransformed;

	public CryptoStream(Stream stream, ICryptoTransform transform, CryptoStreamMode mode)
	{
		_stream = stream;
		_transformMode = mode;
		_Transform = transform;
		switch (_transformMode)
		{
		case CryptoStreamMode.Read:
			if (!_stream.CanRead)
			{
				throw new ArgumentException(Environment.GetResourceString("Stream was not readable."), "stream");
			}
			_canRead = true;
			break;
		case CryptoStreamMode.Write:
			if (!_stream.CanWrite)
			{
				throw new ArgumentException(Environment.GetResourceString("Stream was not writable."), "stream");
			}
			_canWrite = true;
			break;
		default:
			throw new ArgumentException(Environment.GetResourceString("Value was invalid."));
		}
		InitializeBuffer();
	}

	public void FlushFinalBlock()
	{
		if (_finalBlockTransformed)
		{
			throw new NotSupportedException(Environment.GetResourceString("FlushFinalBlock() method was called twice on a CryptoStream. It can only be called once."));
		}
		byte[] array = _Transform.TransformFinalBlock(_InputBuffer, 0, _InputBufferIndex);
		_finalBlockTransformed = true;
		if (_canWrite && _OutputBufferIndex > 0)
		{
			_stream.Write(_OutputBuffer, 0, _OutputBufferIndex);
			_OutputBufferIndex = 0;
		}
		if (_canWrite)
		{
			_stream.Write(array, 0, array.Length);
		}
		if (_stream is CryptoStream cryptoStream)
		{
			if (!cryptoStream.HasFlushedFinalBlock)
			{
				cryptoStream.FlushFinalBlock();
			}
		}
		else
		{
			_stream.Flush();
		}
		if (_InputBuffer != null)
		{
			Array.Clear(_InputBuffer, 0, _InputBuffer.Length);
		}
		if (_OutputBuffer != null)
		{
			Array.Clear(_OutputBuffer, 0, _OutputBuffer.Length);
		}
	}

	public override void Flush()
	{
	}

	public override Task FlushAsync(CancellationToken cancellationToken)
	{
		if (GetType() != typeof(CryptoStream))
		{
			return base.FlushAsync(cancellationToken);
		}
		if (!cancellationToken.IsCancellationRequested)
		{
			return Task.CompletedTask;
		}
		return Task.FromCancellation(cancellationToken);
	}

	public override long Seek(long offset, SeekOrigin origin)
	{
		throw new NotSupportedException(Environment.GetResourceString("Stream does not support seeking."));
	}

	public override void SetLength(long value)
	{
		throw new NotSupportedException(Environment.GetResourceString("Stream does not support seeking."));
	}

	public override int Read([In][Out] byte[] buffer, int offset, int count)
	{
		if (!CanRead)
		{
			throw new NotSupportedException(Environment.GetResourceString("Stream does not support reading."));
		}
		if (offset < 0)
		{
			throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("Non-negative number required."));
		}
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Non-negative number required."));
		}
		if (buffer.Length - offset < count)
		{
			throw new ArgumentException(Environment.GetResourceString("Offset and length were out of bounds for the array or count is greater than the number of elements from index to the end of the source collection."));
		}
		int num = count;
		int num2 = offset;
		if (_OutputBufferIndex != 0)
		{
			if (_OutputBufferIndex > count)
			{
				Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, offset, count);
				Buffer.InternalBlockCopy(_OutputBuffer, count, _OutputBuffer, 0, _OutputBufferIndex - count);
				_OutputBufferIndex -= count;
				return count;
			}
			Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, offset, _OutputBufferIndex);
			num -= _OutputBufferIndex;
			num2 += _OutputBufferIndex;
			_OutputBufferIndex = 0;
		}
		if (_finalBlockTransformed)
		{
			return count - num;
		}
		int num3 = 0;
		if (num > _OutputBlockSize && _Transform.CanTransformMultipleBlocks)
		{
			int num4 = num / _OutputBlockSize * _InputBlockSize;
			byte[] array = new byte[num4];
			Buffer.InternalBlockCopy(_InputBuffer, 0, array, 0, _InputBufferIndex);
			num3 = _InputBufferIndex;
			num3 += _stream.Read(array, _InputBufferIndex, num4 - _InputBufferIndex);
			_InputBufferIndex = 0;
			if (num3 <= _InputBlockSize)
			{
				_InputBuffer = array;
				_InputBufferIndex = num3;
			}
			else
			{
				int num5 = num3 / _InputBlockSize * _InputBlockSize;
				int num6 = num3 - num5;
				if (num6 != 0)
				{
					_InputBufferIndex = num6;
					Buffer.InternalBlockCopy(array, num5, _InputBuffer, 0, num6);
				}
				byte[] array2 = new byte[num5 / _InputBlockSize * _OutputBlockSize];
				int num7 = _Transform.TransformBlock(array, 0, num5, array2, 0);
				Buffer.InternalBlockCopy(array2, 0, buffer, num2, num7);
				Array.Clear(array, 0, array.Length);
				Array.Clear(array2, 0, array2.Length);
				num -= num7;
				num2 += num7;
			}
		}
		while (num > 0)
		{
			while (_InputBufferIndex < _InputBlockSize)
			{
				num3 = _stream.Read(_InputBuffer, _InputBufferIndex, _InputBlockSize - _InputBufferIndex);
				if (num3 != 0)
				{
					_InputBufferIndex += num3;
					continue;
				}
				_OutputBufferIndex = (_OutputBuffer = _Transform.TransformFinalBlock(_InputBuffer, 0, _InputBufferIndex)).Length;
				_finalBlockTransformed = true;
				if (num < _OutputBufferIndex)
				{
					Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, num2, num);
					_OutputBufferIndex -= num;
					Buffer.InternalBlockCopy(_OutputBuffer, num, _OutputBuffer, 0, _OutputBufferIndex);
					return count;
				}
				Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, num2, _OutputBufferIndex);
				num -= _OutputBufferIndex;
				_OutputBufferIndex = 0;
				return count - num;
			}
			int num7 = _Transform.TransformBlock(_InputBuffer, 0, _InputBlockSize, _OutputBuffer, 0);
			_InputBufferIndex = 0;
			if (num >= num7)
			{
				Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, num2, num7);
				num2 += num7;
				num -= num7;
				continue;
			}
			Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, num2, num);
			_OutputBufferIndex = num7 - num;
			Buffer.InternalBlockCopy(_OutputBuffer, num, _OutputBuffer, 0, _OutputBufferIndex);
			return count;
		}
		return count;
	}

	public override Task<int> ReadAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
	{
		if (!CanRead)
		{
			throw new NotSupportedException(Environment.GetResourceString("Stream does not support reading."));
		}
		if (offset < 0)
		{
			throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("Non-negative number required."));
		}
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Non-negative number required."));
		}
		if (buffer.Length - offset < count)
		{
			throw new ArgumentException(Environment.GetResourceString("Offset and length were out of bounds for the array or count is greater than the number of elements from index to the end of the source collection."));
		}
		if (GetType() != typeof(CryptoStream))
		{
			return base.ReadAsync(buffer, offset, count, cancellationToken);
		}
		if (cancellationToken.IsCancellationRequested)
		{
			return Task.FromCancellation<int>(cancellationToken);
		}
		return ReadAsyncInternal(buffer, offset, count, cancellationToken);
	}

	private async Task<int> ReadAsyncInternal(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
	{
		await default(HopToThreadPoolAwaitable);
		SemaphoreSlim sem = EnsureAsyncActiveSemaphoreInitialized();
		await sem.WaitAsync().ConfigureAwait(continueOnCapturedContext: false);
		try
		{
			int bytesToDeliver2 = count;
			int currentOutputIndex = offset;
			if (_OutputBufferIndex != 0)
			{
				if (_OutputBufferIndex > count)
				{
					Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, offset, count);
					Buffer.InternalBlockCopy(_OutputBuffer, count, _OutputBuffer, 0, _OutputBufferIndex - count);
					_OutputBufferIndex -= count;
					return count;
				}
				Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, offset, _OutputBufferIndex);
				bytesToDeliver2 -= _OutputBufferIndex;
				currentOutputIndex += _OutputBufferIndex;
				_OutputBufferIndex = 0;
			}
			if (_finalBlockTransformed)
			{
				return count - bytesToDeliver2;
			}
			if (bytesToDeliver2 > _OutputBlockSize && _Transform.CanTransformMultipleBlocks)
			{
				int num = bytesToDeliver2 / _OutputBlockSize * _InputBlockSize;
				byte[] tempInputBuffer = new byte[num];
				Buffer.InternalBlockCopy(_InputBuffer, 0, tempInputBuffer, 0, _InputBufferIndex);
				int inputBufferIndex = _InputBufferIndex;
				int num2 = inputBufferIndex;
				inputBufferIndex = num2 + await _stream.ReadAsync(tempInputBuffer, _InputBufferIndex, num - _InputBufferIndex, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
				_InputBufferIndex = 0;
				if (inputBufferIndex <= _InputBlockSize)
				{
					_InputBuffer = tempInputBuffer;
					_InputBufferIndex = inputBufferIndex;
				}
				else
				{
					int num3 = inputBufferIndex / _InputBlockSize * _InputBlockSize;
					int num4 = inputBufferIndex - num3;
					if (num4 != 0)
					{
						_InputBufferIndex = num4;
						Buffer.InternalBlockCopy(tempInputBuffer, num3, _InputBuffer, 0, num4);
					}
					byte[] array = new byte[num3 / _InputBlockSize * _OutputBlockSize];
					int num5 = _Transform.TransformBlock(tempInputBuffer, 0, num3, array, 0);
					Buffer.InternalBlockCopy(array, 0, buffer, currentOutputIndex, num5);
					Array.Clear(tempInputBuffer, 0, tempInputBuffer.Length);
					Array.Clear(array, 0, array.Length);
					bytesToDeliver2 -= num5;
					currentOutputIndex += num5;
				}
			}
			while (bytesToDeliver2 > 0)
			{
				while (_InputBufferIndex < _InputBlockSize)
				{
					int inputBufferIndex = await _stream.ReadAsync(_InputBuffer, _InputBufferIndex, _InputBlockSize - _InputBufferIndex, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
					if (inputBufferIndex != 0)
					{
						_InputBufferIndex += inputBufferIndex;
						continue;
					}
					_OutputBufferIndex = (_OutputBuffer = _Transform.TransformFinalBlock(_InputBuffer, 0, _InputBufferIndex)).Length;
					_finalBlockTransformed = true;
					if (bytesToDeliver2 < _OutputBufferIndex)
					{
						Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, currentOutputIndex, bytesToDeliver2);
						_OutputBufferIndex -= bytesToDeliver2;
						Buffer.InternalBlockCopy(_OutputBuffer, bytesToDeliver2, _OutputBuffer, 0, _OutputBufferIndex);
						return count;
					}
					Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, currentOutputIndex, _OutputBufferIndex);
					bytesToDeliver2 -= _OutputBufferIndex;
					_OutputBufferIndex = 0;
					return count - bytesToDeliver2;
				}
				int num5 = _Transform.TransformBlock(_InputBuffer, 0, _InputBlockSize, _OutputBuffer, 0);
				_InputBufferIndex = 0;
				if (bytesToDeliver2 >= num5)
				{
					Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, currentOutputIndex, num5);
					currentOutputIndex += num5;
					bytesToDeliver2 -= num5;
					continue;
				}
				Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, currentOutputIndex, bytesToDeliver2);
				_OutputBufferIndex = num5 - bytesToDeliver2;
				Buffer.InternalBlockCopy(_OutputBuffer, bytesToDeliver2, _OutputBuffer, 0, _OutputBufferIndex);
				return count;
			}
			return count;
		}
		finally
		{
			sem.Release();
		}
	}

	public override void Write(byte[] buffer, int offset, int count)
	{
		if (!CanWrite)
		{
			throw new NotSupportedException(Environment.GetResourceString("Stream does not support writing."));
		}
		if (offset < 0)
		{
			throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("Non-negative number required."));
		}
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Non-negative number required."));
		}
		if (buffer.Length - offset < count)
		{
			throw new ArgumentException(Environment.GetResourceString("Offset and length were out of bounds for the array or count is greater than the number of elements from index to the end of the source collection."));
		}
		int num = count;
		int num2 = offset;
		if (_InputBufferIndex > 0)
		{
			if (count < _InputBlockSize - _InputBufferIndex)
			{
				Buffer.InternalBlockCopy(buffer, offset, _InputBuffer, _InputBufferIndex, count);
				_InputBufferIndex += count;
				return;
			}
			Buffer.InternalBlockCopy(buffer, offset, _InputBuffer, _InputBufferIndex, _InputBlockSize - _InputBufferIndex);
			num2 += _InputBlockSize - _InputBufferIndex;
			num -= _InputBlockSize - _InputBufferIndex;
			_InputBufferIndex = _InputBlockSize;
		}
		if (_OutputBufferIndex > 0)
		{
			_stream.Write(_OutputBuffer, 0, _OutputBufferIndex);
			_OutputBufferIndex = 0;
		}
		if (_InputBufferIndex == _InputBlockSize)
		{
			int count2 = _Transform.TransformBlock(_InputBuffer, 0, _InputBlockSize, _OutputBuffer, 0);
			_stream.Write(_OutputBuffer, 0, count2);
			_InputBufferIndex = 0;
		}
		while (num > 0)
		{
			if (num >= _InputBlockSize)
			{
				if (_Transform.CanTransformMultipleBlocks)
				{
					int num3 = num / _InputBlockSize;
					int num4 = num3 * _InputBlockSize;
					byte[] array = new byte[num3 * _OutputBlockSize];
					int count2 = _Transform.TransformBlock(buffer, num2, num4, array, 0);
					_stream.Write(array, 0, count2);
					num2 += num4;
					num -= num4;
				}
				else
				{
					int count2 = _Transform.TransformBlock(buffer, num2, _InputBlockSize, _OutputBuffer, 0);
					_stream.Write(_OutputBuffer, 0, count2);
					num2 += _InputBlockSize;
					num -= _InputBlockSize;
				}
				continue;
			}
			Buffer.InternalBlockCopy(buffer, num2, _InputBuffer, 0, num);
			_InputBufferIndex += num;
			break;
		}
	}

	public override Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
	{
		if (!CanWrite)
		{
			throw new NotSupportedException(Environment.GetResourceString("Stream does not support writing."));
		}
		if (offset < 0)
		{
			throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("Non-negative number required."));
		}
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Non-negative number required."));
		}
		if (buffer.Length - offset < count)
		{
			throw new ArgumentException(Environment.GetResourceString("Offset and length were out of bounds for the array or count is greater than the number of elements from index to the end of the source collection."));
		}
		if (GetType() != typeof(CryptoStream))
		{
			return base.WriteAsync(buffer, offset, count, cancellationToken);
		}
		if (cancellationToken.IsCancellationRequested)
		{
			return Task.FromCancellation(cancellationToken);
		}
		return WriteAsyncInternal(buffer, offset, count, cancellationToken);
	}

	private async Task WriteAsyncInternal(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
	{
		await default(HopToThreadPoolAwaitable);
		SemaphoreSlim sem = EnsureAsyncActiveSemaphoreInitialized();
		await sem.WaitAsync().ConfigureAwait(continueOnCapturedContext: false);
		try
		{
			int bytesToWrite = count;
			int currentInputIndex = offset;
			if (_InputBufferIndex > 0)
			{
				if (count < _InputBlockSize - _InputBufferIndex)
				{
					Buffer.InternalBlockCopy(buffer, offset, _InputBuffer, _InputBufferIndex, count);
					_InputBufferIndex += count;
					return;
				}
				Buffer.InternalBlockCopy(buffer, offset, _InputBuffer, _InputBufferIndex, _InputBlockSize - _InputBufferIndex);
				currentInputIndex += _InputBlockSize - _InputBufferIndex;
				bytesToWrite -= _InputBlockSize - _InputBufferIndex;
				_InputBufferIndex = _InputBlockSize;
			}
			if (_OutputBufferIndex > 0)
			{
				await _stream.WriteAsync(_OutputBuffer, 0, _OutputBufferIndex, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
				_OutputBufferIndex = 0;
			}
			if (_InputBufferIndex == _InputBlockSize)
			{
				int count2 = _Transform.TransformBlock(_InputBuffer, 0, _InputBlockSize, _OutputBuffer, 0);
				await _stream.WriteAsync(_OutputBuffer, 0, count2, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
				_InputBufferIndex = 0;
			}
			while (bytesToWrite > 0)
			{
				if (bytesToWrite >= _InputBlockSize)
				{
					if (_Transform.CanTransformMultipleBlocks)
					{
						int num = bytesToWrite / _InputBlockSize;
						int numWholeBlocksInBytes = num * _InputBlockSize;
						byte[] array = new byte[num * _OutputBlockSize];
						int count2 = _Transform.TransformBlock(buffer, currentInputIndex, numWholeBlocksInBytes, array, 0);
						await _stream.WriteAsync(array, 0, count2, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
						currentInputIndex += numWholeBlocksInBytes;
						bytesToWrite -= numWholeBlocksInBytes;
					}
					else
					{
						int count2 = _Transform.TransformBlock(buffer, currentInputIndex, _InputBlockSize, _OutputBuffer, 0);
						await _stream.WriteAsync(_OutputBuffer, 0, count2, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
						currentInputIndex += _InputBlockSize;
						bytesToWrite -= _InputBlockSize;
					}
					continue;
				}
				Buffer.InternalBlockCopy(buffer, currentInputIndex, _InputBuffer, 0, bytesToWrite);
				_InputBufferIndex += bytesToWrite;
				break;
			}
		}
		finally
		{
			sem.Release();
		}
	}

	public void Clear()
	{
		Close();
	}

	protected override void Dispose(bool disposing)
	{
		try
		{
			if (disposing)
			{
				if (!_finalBlockTransformed)
				{
					FlushFinalBlock();
				}
				_stream.Close();
			}
		}
		finally
		{
			try
			{
				_finalBlockTransformed = true;
				if (_InputBuffer != null)
				{
					Array.Clear(_InputBuffer, 0, _InputBuffer.Length);
				}
				if (_OutputBuffer != null)
				{
					Array.Clear(_OutputBuffer, 0, _OutputBuffer.Length);
				}
				_InputBuffer = null;
				_OutputBuffer = null;
				_canRead = false;
				_canWrite = false;
			}
			finally
			{
				base.Dispose(disposing);
			}
		}
	}

	private void InitializeBuffer()
	{
		if (_Transform != null)
		{
			_InputBlockSize = _Transform.InputBlockSize;
			_InputBuffer = new byte[_InputBlockSize];
			_OutputBlockSize = _Transform.OutputBlockSize;
			_OutputBuffer = new byte[_OutputBlockSize];
		}
	}
}
