using System.IO;
using System.Threading.Tasks;

namespace System.Net.Http;

public class ByteArrayContent : HttpContent
{
	private readonly byte[] content;

	private readonly int offset;

	private readonly int count;

	public ByteArrayContent(byte[] content)
	{
		if (content == null)
		{
			throw new ArgumentNullException("content");
		}
		this.content = content;
		count = content.Length;
	}

	public ByteArrayContent(byte[] content, int offset, int count)
		: this(content)
	{
		if (offset < 0 || offset > this.count)
		{
			throw new ArgumentOutOfRangeException("offset");
		}
		if (count < 0 || count > this.count - offset)
		{
			throw new ArgumentOutOfRangeException("count");
		}
		this.offset = offset;
		this.count = count;
	}

	protected override Task<Stream> CreateContentReadStreamAsync()
	{
		return Task.FromResult((Stream)new MemoryStream(content, offset, count));
	}

	protected internal override Task SerializeToStreamAsync(Stream stream, TransportContext context)
	{
		return stream.WriteAsync(content, offset, count);
	}

	protected internal override bool TryComputeLength(out long length)
	{
		length = count;
		return true;
	}
}
