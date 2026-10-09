using System.Collections.Generic;

namespace System.Net.Http.Headers;

public sealed class HttpRequestHeaders : HttpHeaders
{
	private bool? expectContinue;

	public HttpHeaderValueCollection<MediaTypeWithQualityHeaderValue> Accept => GetValues<MediaTypeWithQualityHeaderValue>("Accept");

	public HttpHeaderValueCollection<StringWithQualityHeaderValue> AcceptCharset => GetValues<StringWithQualityHeaderValue>("Accept-Charset");

	public HttpHeaderValueCollection<StringWithQualityHeaderValue> AcceptEncoding => GetValues<StringWithQualityHeaderValue>("Accept-Encoding");

	public HttpHeaderValueCollection<StringWithQualityHeaderValue> AcceptLanguage => GetValues<StringWithQualityHeaderValue>("Accept-Language");

	public AuthenticationHeaderValue Authorization
	{
		get
		{
			return GetValue<AuthenticationHeaderValue>("Authorization");
		}
		set
		{
			AddOrRemove("Authorization", value);
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

	internal bool ConnectionKeepAlive => Connection.Find((string l) => string.Equals(l, "Keep-Alive", StringComparison.OrdinalIgnoreCase)) != null;

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

	public HttpHeaderValueCollection<NameValueWithParametersHeaderValue> Expect => GetValues<NameValueWithParametersHeaderValue>("Expect");

	public bool? ExpectContinue
	{
		get
		{
			if (expectContinue.HasValue)
			{
				return expectContinue;
			}
			if (TransferEncoding.Find((TransferCodingHeaderValue l) => string.Equals(l.Value, "100-continue", StringComparison.OrdinalIgnoreCase)) == null)
			{
				return null;
			}
			return true;
		}
		set
		{
			if (expectContinue != value)
			{
				Expect.Remove((NameValueWithParametersHeaderValue l) => l.Name == "100-continue");
				if (value == true)
				{
					Expect.Add(new NameValueWithParametersHeaderValue("100-continue"));
				}
				expectContinue = value;
			}
		}
	}

	public string From
	{
		get
		{
			return GetValue<string>("From");
		}
		set
		{
			if (!string.IsNullOrEmpty(value) && !Parser.EmailAddress.TryParse(value, out value))
			{
				throw new FormatException();
			}
			AddOrRemove("From", value);
		}
	}

	public string Host
	{
		get
		{
			return GetValue<string>("Host");
		}
		set
		{
			AddOrRemove("Host", value);
		}
	}

	public HttpHeaderValueCollection<EntityTagHeaderValue> IfMatch => GetValues<EntityTagHeaderValue>("If-Match");

	public DateTimeOffset? IfModifiedSince
	{
		get
		{
			return GetValue<DateTimeOffset?>("If-Modified-Since");
		}
		set
		{
			AddOrRemove("If-Modified-Since", value, Parser.DateTime.ToString);
		}
	}

	public HttpHeaderValueCollection<EntityTagHeaderValue> IfNoneMatch => GetValues<EntityTagHeaderValue>("If-None-Match");

	public RangeConditionHeaderValue IfRange
	{
		get
		{
			return GetValue<RangeConditionHeaderValue>("If-Range");
		}
		set
		{
			AddOrRemove("If-Range", value);
		}
	}

	public DateTimeOffset? IfUnmodifiedSince
	{
		get
		{
			return GetValue<DateTimeOffset?>("If-Unmodified-Since");
		}
		set
		{
			AddOrRemove("If-Unmodified-Since", value, Parser.DateTime.ToString);
		}
	}

	public int? MaxForwards
	{
		get
		{
			return GetValue<int?>("Max-Forwards");
		}
		set
		{
			AddOrRemove("Max-Forwards", value);
		}
	}

	public HttpHeaderValueCollection<NameValueHeaderValue> Pragma => GetValues<NameValueHeaderValue>("Pragma");

	public AuthenticationHeaderValue ProxyAuthorization
	{
		get
		{
			return GetValue<AuthenticationHeaderValue>("Proxy-Authorization");
		}
		set
		{
			AddOrRemove("Proxy-Authorization", value);
		}
	}

	public RangeHeaderValue Range
	{
		get
		{
			return GetValue<RangeHeaderValue>("Range");
		}
		set
		{
			AddOrRemove("Range", value);
		}
	}

	public Uri Referrer
	{
		get
		{
			return GetValue<Uri>("Referer");
		}
		set
		{
			AddOrRemove("Referer", value);
		}
	}

	public HttpHeaderValueCollection<TransferCodingWithQualityHeaderValue> TE => GetValues<TransferCodingWithQualityHeaderValue>("TE");

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
			if (TransferEncoding.Find((TransferCodingHeaderValue l) => string.Equals(l.Value, "chunked", StringComparison.OrdinalIgnoreCase)) == null)
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

	public HttpHeaderValueCollection<ProductInfoHeaderValue> UserAgent => GetValues<ProductInfoHeaderValue>("User-Agent");

	public HttpHeaderValueCollection<ViaHeaderValue> Via => GetValues<ViaHeaderValue>("Via");

	public HttpHeaderValueCollection<WarningHeaderValue> Warning => GetValues<WarningHeaderValue>("Warning");

	internal HttpRequestHeaders()
		: base(HttpHeaderKind.Request)
	{
	}

	internal void AddHeaders(HttpRequestHeaders headers)
	{
		foreach (KeyValuePair<string, IEnumerable<string>> header in headers)
		{
			TryAddWithoutValidation(header.Key, header.Value);
		}
	}
}
