using System.IO;
using System.Net.Http.Headers;
using System.Text;
using System.Threading.Tasks;

namespace System.Net.Http;

public abstract class HttpContent : IDisposable
{
	private sealed class FixedMemoryStream : MemoryStream
	{
		private readonly long maxSize;

		public FixedMemoryStream(long maxSize)
		{
			this.maxSize = maxSize;
		}

		private void CheckOverflow(int count)
		{
			if (Length + count > maxSize)
			{
				throw new HttpRequestException($"Cannot write more bytes to the buffer than the configured maximum buffer size: {maxSize}");
			}
		}

		public override void WriteByte(byte value)
		{
			CheckOverflow(1);
			base.WriteByte(value);
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			CheckOverflow(count);
			base.Write(buffer, offset, count);
		}
	}

	private FixedMemoryStream buffer;

	private Stream stream;

	private bool disposed;

	private HttpContentHeaders headers;

	public HttpContentHeaders Headers => headers ?? (headers = new HttpContentHeaders(this));

	internal long? LoadedBufferLength
	{
		get
		{
			if (buffer != null)
			{
				return buffer.Length;
			}
			return null;
		}
	}

	internal void CopyTo(Stream stream)
	{
		CopyToAsync(stream).Wait();
	}

	public Task CopyToAsync(Stream stream)
	{
		return CopyToAsync(stream, null);
	}

	public Task CopyToAsync(Stream stream, TransportContext context)
	{
		if (stream == null)
		{
			throw new ArgumentNullException("stream");
		}
		if (buffer != null)
		{
			return buffer.CopyToAsync(stream);
		}
		return SerializeToStreamAsync(stream, context);
	}

	protected virtual async Task<Stream> CreateContentReadStreamAsync()
	{
		await LoadIntoBufferAsync().ConfigureAwait(continueOnCapturedContext: false);
		return buffer;
	}

	private static FixedMemoryStream CreateFixedMemoryStream(long maxBufferSize)
	{
		return new FixedMemoryStream(maxBufferSize);
	}

	public void Dispose()
	{
		Dispose(disposing: true);
	}

	protected virtual void Dispose(bool disposing)
	{
		if (disposing && !disposed)
		{
			disposed = true;
			if (buffer != null)
			{
				buffer.Dispose();
			}
		}
	}

	public Task LoadIntoBufferAsync()
	{
		return LoadIntoBufferAsync(2147483647L);
	}

	public async Task LoadIntoBufferAsync(long maxBufferSize)
	{
		if (disposed)
		{
			throw new ObjectDisposedException(GetType().ToString());
		}
		if (buffer == null)
		{
			buffer = CreateFixedMemoryStream(maxBufferSize);
			await SerializeToStreamAsync(buffer, null).ConfigureAwait(continueOnCapturedContext: false);
			buffer.Seek(0L, SeekOrigin.Begin);
		}
	}

	public async Task<Stream> ReadAsStreamAsync()
	{
		if (disposed)
		{
			throw new ObjectDisposedException(GetType().ToString());
		}
		if (buffer != null)
		{
			return new MemoryStream(buffer.GetBuffer(), 0, (int)buffer.Length, writable: false);
		}
		if (stream == null)
		{
			stream = await CreateContentReadStreamAsync().ConfigureAwait(continueOnCapturedContext: false);
		}
		return stream;
	}

	public async Task<byte[]> ReadAsByteArrayAsync()
	{
		await LoadIntoBufferAsync().ConfigureAwait(continueOnCapturedContext: false);
		return buffer.ToArray();
	}

	public async Task<string> ReadAsStringAsync()
	{
		await LoadIntoBufferAsync().ConfigureAwait(continueOnCapturedContext: false);
		if (buffer.Length == 0L)
		{
			return string.Empty;
		}
		byte[] array = buffer.GetBuffer();
		int num = (int)buffer.Length;
		int preambleLength = 0;
		Encoding encoding;
		if (headers != null && headers.ContentType != null && headers.ContentType.CharSet != null)
		{
			encoding = Encoding.GetEncoding(headers.ContentType.CharSet);
			preambleLength = StartsWith(array, num, encoding.GetPreamble());
		}
		else
		{
			encoding = GetEncodingFromBuffer(array, num, ref preambleLength) ?? Encoding.UTF8;
		}
		return encoding.GetString(array, preambleLength, num - preambleLength);
	}

	private static Encoding GetEncodingFromBuffer(byte[] buffer, int length, ref int preambleLength)
	{
		Encoding[] array = new Encoding[3]
		{
			Encoding.UTF8,
			Encoding.UTF32,
			Encoding.Unicode
		};
		foreach (Encoding encoding in array)
		{
			if ((preambleLength = StartsWith(buffer, length, encoding.GetPreamble())) != 0)
			{
				return encoding;
			}
		}
		return null;
	}

	private static int StartsWith(byte[] array, int length, byte[] value)
	{
		if (length < value.Length)
		{
			return 0;
		}
		for (int i = 0; i < value.Length; i++)
		{
			if (array[i] != value[i])
			{
				return 0;
			}
		}
		return value.Length;
	}

	protected internal abstract Task SerializeToStreamAsync(Stream stream, TransportContext context);

	protected internal abstract bool TryComputeLength(out long length);
}
