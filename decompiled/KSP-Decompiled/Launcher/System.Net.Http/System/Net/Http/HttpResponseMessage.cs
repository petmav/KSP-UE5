using System.Net.Http.Headers;
using System.Text;

namespace System.Net.Http;

public class HttpResponseMessage : IDisposable
{
	private HttpResponseHeaders headers;

	private string reasonPhrase;

	private HttpStatusCode statusCode;

	private Version version;

	private bool disposed;

	public HttpContent Content { get; set; }

	public HttpResponseHeaders Headers => headers ?? (headers = new HttpResponseHeaders());

	public bool IsSuccessStatusCode
	{
		get
		{
			if (statusCode >= HttpStatusCode.OK)
			{
				return statusCode < HttpStatusCode.MultipleChoices;
			}
			return false;
		}
	}

	public string ReasonPhrase
	{
		get
		{
			return reasonPhrase ?? HttpStatusDescription.Get(statusCode);
		}
		set
		{
			reasonPhrase = value;
		}
	}

	public HttpRequestMessage RequestMessage { get; set; }

	public HttpStatusCode StatusCode
	{
		get
		{
			return statusCode;
		}
		set
		{
			if (value < (HttpStatusCode)0)
			{
				throw new ArgumentOutOfRangeException();
			}
			statusCode = value;
		}
	}

	public Version Version
	{
		get
		{
			return version ?? HttpVersion.Version11;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("Version");
			}
			version = value;
		}
	}

	public HttpResponseMessage()
		: this(HttpStatusCode.OK)
	{
	}

	public HttpResponseMessage(HttpStatusCode statusCode)
	{
		StatusCode = statusCode;
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
			if (Content != null)
			{
				Content.Dispose();
			}
		}
	}

	public HttpResponseMessage EnsureSuccessStatusCode()
	{
		if (IsSuccessStatusCode)
		{
			return this;
		}
		throw new HttpRequestException($"{(int)statusCode} ({ReasonPhrase})");
	}

	public override string ToString()
	{
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("StatusCode: ").Append((int)StatusCode);
		stringBuilder.Append(", ReasonPhrase: '").Append(ReasonPhrase ?? "<null>");
		stringBuilder.Append("', Version: ").Append(Version);
		stringBuilder.Append(", Content: ").Append((Content != null) ? Content.ToString() : "<null>");
		stringBuilder.Append(", Headers:\r\n{\r\n").Append(Headers);
		if (Content != null)
		{
			stringBuilder.Append(Content.Headers);
		}
		stringBuilder.Append("}");
		return stringBuilder.ToString();
	}
}
