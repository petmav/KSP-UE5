using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace System.Net.Http;

public class StreamContent : HttpContent
{
	private readonly Stream content;

	private readonly int bufferSize;

	private readonly CancellationToken cancellationToken;

	private readonly long startPosition;

	private bool contentCopied;

	public StreamContent(Stream content)
		: this(content, 16384)
	{
	}

	public StreamContent(Stream content, int bufferSize)
	{
		if (content == null)
		{
			throw new ArgumentNullException("content");
		}
		if (bufferSize <= 0)
		{
			throw new ArgumentOutOfRangeException("bufferSize");
		}
		this.content = content;
		this.bufferSize = bufferSize;
		if (content.CanSeek)
		{
			startPosition = content.Position;
		}
	}

	internal StreamContent(Stream content, CancellationToken cancellationToken)
		: this(content)
	{
		this.cancellationToken = cancellationToken;
	}

	protected override Task<Stream> CreateContentReadStreamAsync()
	{
		return Task.FromResult(content);
	}

	protected override void Dispose(bool disposing)
	{
		if (disposing)
		{
			content.Dispose();
		}
		base.Dispose(disposing);
	}

	protected internal override Task SerializeToStreamAsync(Stream stream, TransportContext context)
	{
		if (contentCopied)
		{
			if (!content.CanSeek)
			{
				throw new InvalidOperationException("The stream was already consumed. It cannot be read again.");
			}
			content.Seek(startPosition, SeekOrigin.Begin);
		}
		else
		{
			contentCopied = true;
		}
		return content.CopyToAsync(stream, bufferSize, cancellationToken);
	}

	protected internal override bool TryComputeLength(out long length)
	{
		if (!content.CanSeek)
		{
			length = 0L;
			return false;
		}
		length = content.Length - startPosition;
		return true;
	}
}
