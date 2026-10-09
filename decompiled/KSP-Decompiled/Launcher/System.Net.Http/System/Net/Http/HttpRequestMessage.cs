using System.Collections.Generic;
using System.Net.Http.Headers;
using System.Text;

namespace System.Net.Http;

public class HttpRequestMessage : IDisposable
{
	private HttpRequestHeaders headers;

	private HttpMethod method;

	private Version version;

	private Dictionary<string, object> properties;

	private Uri uri;

	private bool is_used;

	private bool disposed;

	public HttpContent Content { get; set; }

	public HttpRequestHeaders Headers => headers ?? (headers = new HttpRequestHeaders());

	public HttpMethod Method
	{
		get
		{
			return method;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("method");
			}
			method = value;
		}
	}

	public IDictionary<string, object> Properties => properties ?? (properties = new Dictionary<string, object>());

	public Uri RequestUri
	{
		get
		{
			return uri;
		}
		set
		{
			if (value != null && value.IsAbsoluteUri && !IsAllowedAbsoluteUri(value))
			{
				throw new ArgumentException("Only http or https scheme is allowed");
			}
			uri = value;
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

	public HttpRequestMessage()
	{
		method = HttpMethod.Get;
	}

	public HttpRequestMessage(HttpMethod method, string requestUri)
		: this(method, string.IsNullOrEmpty(requestUri) ? null : new Uri(requestUri, UriKind.RelativeOrAbsolute))
	{
	}

	public HttpRequestMessage(HttpMethod method, Uri requestUri)
	{
		Method = method;
		RequestUri = requestUri;
	}

	private static bool IsAllowedAbsoluteUri(Uri uri)
	{
		if (uri.Scheme == Uri.UriSchemeHttp || uri.Scheme == Uri.UriSchemeHttps)
		{
			return true;
		}
		if (uri.Scheme == Uri.UriSchemeFile && uri.OriginalString.StartsWith("/", StringComparison.Ordinal))
		{
			return true;
		}
		return false;
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

	internal bool SetIsUsed()
	{
		if (is_used)
		{
			return true;
		}
		is_used = true;
		return false;
	}

	public override string ToString()
	{
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("Method: ").Append(method);
		stringBuilder.Append(", RequestUri: '").Append((RequestUri != null) ? RequestUri.ToString() : "<null>");
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
