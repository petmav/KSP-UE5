using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;
using System.Threading;
using System.Threading.Tasks;

namespace System.IO;

public class UnmanagedMemoryStream : Stream
{
	private const long UnmanagedMemStreamMaxLength = long.MaxValue;

	[SecurityCritical]
	private SafeBuffer _buffer;

	[SecurityCritical]
	private unsafe byte* _mem;

	private long _length;

	private long _capacity;

	private long _position;

	private long _offset;

	private FileAccess _access;

	internal bool _isOpen;

	[NonSerialized]
	private Task<int> _lastReadTask;

	public override bool CanRead
	{
		get
		{
			if (_isOpen)
			{
				return (_access & FileAccess.Read) != 0;
			}
			return false;
		}
	}

	public override bool CanSeek => _isOpen;

	public override bool CanWrite
	{
		get
		{
			if (_isOpen)
			{
				return (_access & FileAccess.Write) != 0;
			}
			return false;
		}
	}

	public override long Length
	{
		get
		{
			if (!_isOpen)
			{
				__Error.StreamIsClosed();
			}
			return Interlocked.Read(ref _length);
		}
	}

	public long Capacity
	{
		get
		{
			if (!_isOpen)
			{
				__Error.StreamIsClosed();
			}
			return _capacity;
		}
	}

	public override long Position
	{
		get
		{
			if (!CanSeek)
			{
				__Error.StreamIsClosed();
			}
			return Interlocked.Read(ref _position);
		}
		[SecuritySafeCritical]
		set
		{
			if (value < 0)
			{
				throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("Non-negative number required."));
			}
			if (!CanSeek)
			{
				__Error.StreamIsClosed();
			}
			Interlocked.Exchange(ref _position, value);
		}
	}

	[CLSCompliant(false)]
	public unsafe byte* PositionPointer
	{
		[SecurityCritical]
		get
		{
			if (_buffer != null)
			{
				throw new NotSupportedException(Environment.GetResourceString("This operation is not supported for an UnmanagedMemoryStream created from a SafeBuffer."));
			}
			long num = Interlocked.Read(ref _position);
			if (num > _capacity)
			{
				throw new IndexOutOfRangeException(Environment.GetResourceString("Unmanaged memory stream position was beyond the capacity of the stream."));
			}
			byte* result = _mem + num;
			if (!_isOpen)
			{
				__Error.StreamIsClosed();
			}
			return result;
		}
		[SecurityCritical]
		set
		{
			if (_buffer != null)
			{
				throw new NotSupportedException(Environment.GetResourceString("This operation is not supported for an UnmanagedMemoryStream created from a SafeBuffer."));
			}
			if (!_isOpen)
			{
				__Error.StreamIsClosed();
			}
			if (new IntPtr(value - _mem).ToInt64() > long.MaxValue)
			{
				throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("UnmanagedMemoryStream length must be non-negative and less than 2^63 - 1 - baseAddress."));
			}
			if (value < _mem)
			{
				throw new IOException(Environment.GetResourceString("An attempt was made to move the position before the beginning of the stream."));
			}
			Interlocked.Exchange(ref _position, value - _mem);
		}
	}

	internal unsafe byte* Pointer
	{
		[SecurityCritical]
		get
		{
			if (_buffer != null)
			{
				throw new NotSupportedException(Environment.GetResourceString("This operation is not supported for an UnmanagedMemoryStream created from a SafeBuffer."));
			}
			return _mem;
		}
	}

	[SecuritySafeCritical]
	protected unsafe UnmanagedMemoryStream()
	{
		_mem = null;
		_isOpen = false;
	}

	[SecuritySafeCritical]
	public UnmanagedMemoryStream(SafeBuffer buffer, long offset, long length)
	{
		Initialize(buffer, offset, length, FileAccess.Read, skipSecurityCheck: false);
	}

	[SecuritySafeCritical]
	public UnmanagedMemoryStream(SafeBuffer buffer, long offset, long length, FileAccess access)
	{
		Initialize(buffer, offset, length, access, skipSecurityCheck: false);
	}

	[SecurityCritical]
	internal UnmanagedMemoryStream(SafeBuffer buffer, long offset, long length, FileAccess access, bool skipSecurityCheck)
	{
		Initialize(buffer, offset, length, access, skipSecurityCheck);
	}

	[SecuritySafeCritical]
	protected void Initialize(SafeBuffer buffer, long offset, long length, FileAccess access)
	{
		Initialize(buffer, offset, length, access, skipSecurityCheck: false);
	}

	[SecurityCritical]
	internal unsafe void Initialize(SafeBuffer buffer, long offset, long length, FileAccess access, bool skipSecurityCheck)
	{
		if (buffer == null)
		{
			throw new ArgumentNullException("buffer");
		}
		if (offset < 0)
		{
			throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("Non-negative number required."));
		}
		if (length < 0)
		{
			throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("Non-negative number required."));
		}
		if (buffer.ByteLength < (ulong)(offset + length))
		{
			throw new ArgumentException(Environment.GetResourceString("Offset and length were greater than the size of the SafeBuffer."));
		}
		if (access < FileAccess.Read || access > FileAccess.ReadWrite)
		{
			throw new ArgumentOutOfRangeException("access");
		}
		if (_isOpen)
		{
			throw new InvalidOperationException(Environment.GetResourceString("The method cannot be called twice on the same instance."));
		}
		byte* pointer = null;
		RuntimeHelpers.PrepareConstrainedRegions();
		try
		{
			buffer.AcquirePointer(ref pointer);
			if (pointer + offset + length < pointer)
			{
				throw new ArgumentException(Environment.GetResourceString("The UnmanagedMemoryStream capacity would wrap around the high end of the address space."));
			}
		}
		finally
		{
			if (pointer != null)
			{
				buffer.ReleasePointer();
			}
		}
		_offset = offset;
		_buffer = buffer;
		_length = length;
		_capacity = length;
		_access = access;
		_isOpen = true;
	}

	[SecurityCritical]
	[CLSCompliant(false)]
	public unsafe UnmanagedMemoryStream(byte* pointer, long length)
	{
		Initialize(pointer, length, length, FileAccess.Read, skipSecurityCheck: false);
	}

	[CLSCompliant(false)]
	[SecurityCritical]
	public unsafe UnmanagedMemoryStream(byte* pointer, long length, long capacity, FileAccess access)
	{
		Initialize(pointer, length, capacity, access, skipSecurityCheck: false);
	}

	[SecurityCritical]
	internal unsafe UnmanagedMemoryStream(byte* pointer, long length, long capacity, FileAccess access, bool skipSecurityCheck)
	{
		Initialize(pointer, length, capacity, access, skipSecurityCheck);
	}

	[SecurityCritical]
	[CLSCompliant(false)]
	protected unsafe void Initialize(byte* pointer, long length, long capacity, FileAccess access)
	{
		Initialize(pointer, length, capacity, access, skipSecurityCheck: false);
	}

	[SecurityCritical]
	internal unsafe void Initialize(byte* pointer, long length, long capacity, FileAccess access, bool skipSecurityCheck)
	{
		if (pointer == null)
		{
			throw new ArgumentNullException("pointer");
		}
		if (length < 0 || capacity < 0)
		{
			throw new ArgumentOutOfRangeException((length < 0) ? "length" : "capacity", Environment.GetResourceString("Non-negative number required."));
		}
		if (length > capacity)
		{
			throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("The length cannot be greater than the capacity."));
		}
		if ((nuint)((long)pointer + capacity) < (nuint)pointer)
		{
			throw new ArgumentOutOfRangeException("capacity", Environment.GetResourceString("The UnmanagedMemoryStream capacity would wrap around the high end of the address space."));
		}
		if (access < FileAccess.Read || access > FileAccess.ReadWrite)
		{
			throw new ArgumentOutOfRangeException("access", Environment.GetResourceString("Enum value was out of legal range."));
		}
		if (_isOpen)
		{
			throw new InvalidOperationException(Environment.GetResourceString("The method cannot be called twice on the same instance."));
		}
		_mem = pointer;
		_offset = 0L;
		_length = length;
		_capacity = capacity;
		_access = access;
		_isOpen = true;
	}

	[SecuritySafeCritical]
	protected unsafe override void Dispose(bool disposing)
	{
		_isOpen = false;
		_mem = null;
		base.Dispose(disposing);
	}

	public override void Flush()
	{
		if (!_isOpen)
		{
			__Error.StreamIsClosed();
		}
	}

	[ComVisible(false)]
	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public override Task FlushAsync(CancellationToken cancellationToken)
	{
		if (cancellationToken.IsCancellationRequested)
		{
			return Task.FromCancellation(cancellationToken);
		}
		try
		{
			Flush();
			return Task.CompletedTask;
		}
		catch (Exception exception)
		{
			return Task.FromException(exception);
		}
	}

	[SecuritySafeCritical]
	public unsafe override int Read([In][Out] byte[] buffer, int offset, int count)
	{
		if (buffer == null)
		{
			throw new ArgumentNullException("buffer", Environment.GetResourceString("Buffer cannot be null."));
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
		if (!_isOpen)
		{
			__Error.StreamIsClosed();
		}
		if (!CanRead)
		{
			__Error.ReadNotSupported();
		}
		long num = Interlocked.Read(ref _position);
		long num2 = Interlocked.Read(ref _length) - num;
		if (num2 > count)
		{
			num2 = count;
		}
		if (num2 <= 0)
		{
			return 0;
		}
		int num3 = (int)num2;
		if (num3 < 0)
		{
			num3 = 0;
		}
		if (_buffer != null)
		{
			byte* pointer = null;
			RuntimeHelpers.PrepareConstrainedRegions();
			try
			{
				_buffer.AcquirePointer(ref pointer);
				Buffer.Memcpy(buffer, offset, pointer + num + _offset, 0, num3);
			}
			finally
			{
				if (pointer != null)
				{
					_buffer.ReleasePointer();
				}
			}
		}
		else
		{
			Buffer.Memcpy(buffer, offset, _mem + num, 0, num3);
		}
		Interlocked.Exchange(ref _position, num + num2);
		return num3;
	}

	[ComVisible(false)]
	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public override Task<int> ReadAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
	{
		if (buffer == null)
		{
			throw new ArgumentNullException("buffer", Environment.GetResourceString("Buffer cannot be null."));
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
		if (cancellationToken.IsCancellationRequested)
		{
			return Task.FromCancellation<int>(cancellationToken);
		}
		try
		{
			int num = Read(buffer, offset, count);
			Task<int> lastReadTask = _lastReadTask;
			return (lastReadTask != null && lastReadTask.Result == num) ? lastReadTask : (_lastReadTask = Task.FromResult(num));
		}
		catch (Exception exception)
		{
			return Task.FromException<int>(exception);
		}
	}

	[SecuritySafeCritical]
	public unsafe override int ReadByte()
	{
		if (!_isOpen)
		{
			__Error.StreamIsClosed();
		}
		if (!CanRead)
		{
			__Error.ReadNotSupported();
		}
		long num = Interlocked.Read(ref _position);
		long num2 = Interlocked.Read(ref _length);
		if (num >= num2)
		{
			return -1;
		}
		Interlocked.Exchange(ref _position, num + 1);
		if (_buffer != null)
		{
			byte* pointer = null;
			RuntimeHelpers.PrepareConstrainedRegions();
			try
			{
				_buffer.AcquirePointer(ref pointer);
				return (pointer + num)[_offset];
			}
			finally
			{
				if (pointer != null)
				{
					_buffer.ReleasePointer();
				}
			}
		}
		return _mem[num];
	}

	public override long Seek(long offset, SeekOrigin loc)
	{
		if (!_isOpen)
		{
			__Error.StreamIsClosed();
		}
		if (offset > long.MaxValue)
		{
			throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("UnmanagedMemoryStream length must be non-negative and less than 2^63 - 1 - baseAddress."));
		}
		switch (loc)
		{
		case SeekOrigin.Begin:
			if (offset < 0)
			{
				throw new IOException(Environment.GetResourceString("An attempt was made to move the position before the beginning of the stream."));
			}
			Interlocked.Exchange(ref _position, offset);
			break;
		case SeekOrigin.Current:
		{
			long num2 = Interlocked.Read(ref _position);
			if (offset + num2 < 0)
			{
				throw new IOException(Environment.GetResourceString("An attempt was made to move the position before the beginning of the stream."));
			}
			Interlocked.Exchange(ref _position, offset + num2);
			break;
		}
		case SeekOrigin.End:
		{
			long num = Interlocked.Read(ref _length);
			if (num + offset < 0)
			{
				throw new IOException(Environment.GetResourceString("An attempt was made to move the position before the beginning of the stream."));
			}
			Interlocked.Exchange(ref _position, num + offset);
			break;
		}
		default:
			throw new ArgumentException(Environment.GetResourceString("Invalid seek origin."));
		}
		return Interlocked.Read(ref _position);
	}

	[SecuritySafeCritical]
	public unsafe override void SetLength(long value)
	{
		if (value < 0)
		{
			throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("Non-negative number required."));
		}
		if (_buffer != null)
		{
			throw new NotSupportedException(Environment.GetResourceString("This operation is not supported for an UnmanagedMemoryStream created from a SafeBuffer."));
		}
		if (!_isOpen)
		{
			__Error.StreamIsClosed();
		}
		if (!CanWrite)
		{
			__Error.WriteNotSupported();
		}
		if (value > _capacity)
		{
			throw new IOException(Environment.GetResourceString("Unable to expand length of this stream beyond its capacity."));
		}
		long num = Interlocked.Read(ref _position);
		long num2 = Interlocked.Read(ref _length);
		if (value > num2)
		{
			Buffer.ZeroMemory(_mem + num2, value - num2);
		}
		Interlocked.Exchange(ref _length, value);
		if (num > value)
		{
			Interlocked.Exchange(ref _position, value);
		}
	}

	[SecuritySafeCritical]
	public unsafe override void Write(byte[] buffer, int offset, int count)
	{
		if (buffer == null)
		{
			throw new ArgumentNullException("buffer", Environment.GetResourceString("Buffer cannot be null."));
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
		if (!_isOpen)
		{
			__Error.StreamIsClosed();
		}
		if (!CanWrite)
		{
			__Error.WriteNotSupported();
		}
		long num = Interlocked.Read(ref _position);
		long num2 = Interlocked.Read(ref _length);
		long num3 = num + count;
		if (num3 < 0)
		{
			throw new IOException(Environment.GetResourceString("Stream was too long."));
		}
		if (num3 > _capacity)
		{
			throw new NotSupportedException(Environment.GetResourceString("Unable to expand length of this stream beyond its capacity."));
		}
		if (_buffer == null)
		{
			if (num > num2)
			{
				Buffer.ZeroMemory(_mem + num2, num - num2);
			}
			if (num3 > num2)
			{
				Interlocked.Exchange(ref _length, num3);
			}
		}
		if (_buffer != null)
		{
			if (_capacity - num < count)
			{
				throw new ArgumentException(Environment.GetResourceString("Not enough space available in the buffer."));
			}
			byte* pointer = null;
			RuntimeHelpers.PrepareConstrainedRegions();
			try
			{
				_buffer.AcquirePointer(ref pointer);
				Buffer.Memcpy(pointer + num + _offset, 0, buffer, offset, count);
			}
			finally
			{
				if (pointer != null)
				{
					_buffer.ReleasePointer();
				}
			}
		}
		else
		{
			Buffer.Memcpy(_mem + num, 0, buffer, offset, count);
		}
		Interlocked.Exchange(ref _position, num3);
	}

	[ComVisible(false)]
	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public override Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
	{
		if (buffer == null)
		{
			throw new ArgumentNullException("buffer", Environment.GetResourceString("Buffer cannot be null."));
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
		if (cancellationToken.IsCancellationRequested)
		{
			return Task.FromCancellation(cancellationToken);
		}
		try
		{
			Write(buffer, offset, count);
			return Task.CompletedTask;
		}
		catch (Exception exception)
		{
			return Task.FromException<int>(exception);
		}
	}

	[SecuritySafeCritical]
	public unsafe override void WriteByte(byte value)
	{
		if (!_isOpen)
		{
			__Error.StreamIsClosed();
		}
		if (!CanWrite)
		{
			__Error.WriteNotSupported();
		}
		long num = Interlocked.Read(ref _position);
		long num2 = Interlocked.Read(ref _length);
		long num3 = num + 1;
		if (num >= num2)
		{
			if (num3 < 0)
			{
				throw new IOException(Environment.GetResourceString("Stream was too long."));
			}
			if (num3 > _capacity)
			{
				throw new NotSupportedException(Environment.GetResourceString("Unable to expand length of this stream beyond its capacity."));
			}
			if (_buffer == null)
			{
				if (num > num2)
				{
					Buffer.ZeroMemory(_mem + num2, num - num2);
				}
				Interlocked.Exchange(ref _length, num3);
			}
		}
		if (_buffer != null)
		{
			byte* pointer = null;
			RuntimeHelpers.PrepareConstrainedRegions();
			try
			{
				_buffer.AcquirePointer(ref pointer);
				(pointer + num)[_offset] = value;
			}
			finally
			{
				if (pointer != null)
				{
					_buffer.ReleasePointer();
				}
			}
		}
		else
		{
			_mem[num] = value;
		}
		Interlocked.Exchange(ref _position, num3);
	}
}
