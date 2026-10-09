namespace System.Net.Http.Headers;

public sealed class HttpResponseHeaders : HttpHeaders
{
	public HttpHeaderValueCollection<string> AcceptRanges => GetValues<string>("Accept-Ranges");

	public TimeSpan? Age
	{
		get
		{
			return GetValue<TimeSpan?>("Age");
		}
		set
		{
			AddOrRemove("Age", value, (object l) => ((long)((TimeSpan)l).TotalSeconds).ToString());
		}
	}

	public CacheControlHeaderValue CacheControl
	{
		get
		{
			return GetValue<CacheControlHeaderValue>("Cache-Control");
		}
		set
		{
			AddOrRemove("Cache-Control", value);
		}
	}

	public HttpHeaderValueCollection<string> Connection => GetValues<string>("Connection");

	public bool? ConnectionClose
	{
		get
		{
			if (connectionclose == true || Connection.Find((string l) => string.Equals(l, "close", StringComparison.OrdinalIgnoreCase)) != null)
			{
				return true;
			}
			return connectionclose;
		}
		set
		{
			if (connectionclose != value)
			{
				Connection.Remove("close");
				if (value == true)
				{
					Connection.Add("close");
				}
				connectionclose = value;
			}
		}
	}

	public DateTimeOffset? Date
	{
		get
		{
			return GetValue<DateTimeOffset?>("Date");
		}
		set
		{
			AddOrRemove("Date", value, Parser.DateTime.ToString);
		}
	}

	public EntityTagHeaderValue ETag
	{
		get
		{
			return GetValue<EntityTagHeaderValue>("ETag");
		}
		set
		{
			AddOrRemove("ETag", value);
		}
	}

	public Uri Location
	{
		get
		{
			return GetValue<Uri>("Location");
		}
		set
		{
			AddOrRemove("Location", value);
		}
	}

	public HttpHeaderValueCollection<NameValueHeaderValue> Pragma => GetValues<NameValueHeaderValue>("Pragma");

	public HttpHeaderValueCollection<AuthenticationHeaderValue> ProxyAuthenticate => GetValues<AuthenticationHeaderValue>("Proxy-Authenticate");

	public RetryConditionHeaderValue RetryAfter
	{
		get
		{
			return GetValue<RetryConditionHeaderValue>("Retry-After");
		}
		set
		{
			AddOrRemove("Retry-After", value);
		}
	}

	public HttpHeaderValueCollection<ProductInfoHeaderValue> Server => GetValues<ProductInfoHeaderValue>("Server");

	public HttpHeaderValueCollection<string> Trailer => GetValues<string>("Trailer");

	public HttpHeaderValueCollection<TransferCodingHeaderValue> TransferEncoding => GetValues<TransferCodingHeaderValue>("Transfer-Encoding");

	public bool? TransferEncodingChunked
	{
		get
		{
			if (transferEncodingChunked.HasValue)
			{
				return transferEncodingChunked;
			}
			if (TransferEncoding.Find((TransferCodingHeaderValue l) => StringComparer.OrdinalIgnoreCase.Equals(l.Value, "chunked")) == null)
			{
				return null;
			}
			return true;
		}
		set
		{
			if (value != transferEncodingChunked)
			{
				TransferEncoding.Remove((TransferCodingHeaderValue l) => l.Value == "chunked");
				if (value == true)
				{
					TransferEncoding.Add(new TransferCodingHeaderValue("chunked"));
				}
				transferEncodingChunked = value;
			}
		}
	}

	public HttpHeaderValueCollection<ProductHeaderValue> Upgrade => GetValues<ProductHeaderValue>("Upgrade");

	public HttpHeaderValueCollection<string> Vary => GetValues<string>("Vary");

	public HttpHeaderValueCollection<ViaHeaderValue> Via => GetValues<ViaHeaderValue>("Via");

	public HttpHeaderValueCollection<WarningHeaderValue> Warning => GetValues<WarningHeaderValue>("Warning");

	public HttpHeaderValueCollection<AuthenticationHeaderValue> WwwAuthenticate => GetValues<AuthenticationHeaderValue>("WWW-Authenticate");

	internal HttpResponseHeaders()
		: base(HttpHeaderKind.Response)
	{
	}
}
