using System.Collections.Generic;
using Unity;

namespace System.Net.Http.Headers;

public sealed class HttpContentHeaders : HttpHeaders
{
	private readonly HttpContent content;

	public ICollection<string> Allow => GetValues<string>("Allow");

	public ICollection<string> ContentEncoding => GetValues<string>("Content-Encoding");

	public ContentDispositionHeaderValue ContentDisposition
	{
		get
		{
			return GetValue<ContentDispositionHeaderValue>("Content-Disposition");
		}
		set
		{
			AddOrRemove("Content-Disposition", value);
		}
	}

	public ICollection<string> ContentLanguage => GetValues<string>("Content-Language");

	public long? ContentLength
	{
		get
		{
			long? value = GetValue<long?>("Content-Length");
			if (value.HasValue)
			{
				return value;
			}
			value = content.LoadedBufferLength;
			if (value.HasValue)
			{
				return value;
			}
			if (content.TryComputeLength(out var length))
			{
				SetValue("Content-Length", length);
				return length;
			}
			return null;
		}
		set
		{
			AddOrRemove("Content-Length", value);
		}
	}

	public Uri ContentLocation
	{
		get
		{
			return GetValue<Uri>("Content-Location");
		}
		set
		{
			AddOrRemove("Content-Location", value);
		}
	}

	public byte[] ContentMD5
	{
		get
		{
			return GetValue<byte[]>("Content-MD5");
		}
		set
		{
			AddOrRemove("Content-MD5", value, Parser.MD5.ToString);
		}
	}

	public ContentRangeHeaderValue ContentRange
	{
		get
		{
			return GetValue<ContentRangeHeaderValue>("Content-Range");
		}
		set
		{
			AddOrRemove("Content-Range", value);
		}
	}

	public MediaTypeHeaderValue ContentType
	{
		get
		{
			return GetValue<MediaTypeHeaderValue>("Content-Type");
		}
		set
		{
			AddOrRemove("Content-Type", value);
		}
	}

	public DateTimeOffset? Expires
	{
		get
		{
			return GetValue<DateTimeOffset?>("Expires");
		}
		set
		{
			AddOrRemove("Expires", value, Parser.DateTime.ToString);
		}
	}

	public DateTimeOffset? LastModified
	{
		get
		{
			return GetValue<DateTimeOffset?>("Last-Modified");
		}
		set
		{
			AddOrRemove("Last-Modified", value, Parser.DateTime.ToString);
		}
	}

	internal HttpContentHeaders(HttpContent content)
		: base(HttpHeaderKind.Content)
	{
		this.content = content;
	}

	internal HttpContentHeaders()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
