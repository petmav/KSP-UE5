using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace System.Net.Http.Headers;

public abstract class HttpHeaders : IEnumerable<KeyValuePair<string, IEnumerable<string>>>, IEnumerable
{
	private class HeaderBucket
	{
		public object Parsed;

		private List<string> values;

		public readonly Func<object, string> CustomToString;

		public bool HasStringValues
		{
			get
			{
				if (values != null)
				{
					return values.Count > 0;
				}
				return false;
			}
		}

		public List<string> Values
		{
			get
			{
				return values ?? (values = new List<string>());
			}
			set
			{
				values = value;
			}
		}

		public HeaderBucket(object parsed, Func<object, string> converter)
		{
			Parsed = parsed;
			CustomToString = converter;
		}

		public string ParsedToString()
		{
			if (Parsed == null)
			{
				return null;
			}
			if (CustomToString != null)
			{
				return CustomToString(Parsed);
			}
			return Parsed.ToString();
		}
	}

	private static readonly Dictionary<string, HeaderInfo> known_headers;

	private readonly Dictionary<string, HeaderBucket> headers;

	private readonly HttpHeaderKind HeaderKind;

	internal bool? connectionclose;

	internal bool? transferEncodingChunked;

	static HttpHeaders()
	{
		HeaderInfo[] obj = new HeaderInfo[48]
		{
			HeaderInfo.CreateMulti<MediaTypeWithQualityHeaderValue>("Accept", MediaTypeWithQualityHeaderValue.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateMulti<StringWithQualityHeaderValue>("Accept-Charset", StringWithQualityHeaderValue.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateMulti<StringWithQualityHeaderValue>("Accept-Encoding", StringWithQualityHeaderValue.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateMulti<StringWithQualityHeaderValue>("Accept-Language", StringWithQualityHeaderValue.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateMulti<string>("Accept-Ranges", CollectionParser.TryParse, HttpHeaderKind.Response),
			HeaderInfo.CreateSingle<TimeSpan>("Age", Parser.TimeSpanSeconds.TryParse, HttpHeaderKind.Response),
			HeaderInfo.CreateMulti<string>("Allow", CollectionParser.TryParse, HttpHeaderKind.Content, 0),
			HeaderInfo.CreateSingle<AuthenticationHeaderValue>("Authorization", AuthenticationHeaderValue.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateSingle<CacheControlHeaderValue>("Cache-Control", CacheControlHeaderValue.TryParse, HttpHeaderKind.Request | HttpHeaderKind.Response),
			HeaderInfo.CreateMulti<string>("Connection", CollectionParser.TryParse, HttpHeaderKind.Request | HttpHeaderKind.Response),
			HeaderInfo.CreateSingle<ContentDispositionHeaderValue>("Content-Disposition", ContentDispositionHeaderValue.TryParse, HttpHeaderKind.Content),
			HeaderInfo.CreateMulti<string>("Content-Encoding", CollectionParser.TryParse, HttpHeaderKind.Content),
			HeaderInfo.CreateMulti<string>("Content-Language", CollectionParser.TryParse, HttpHeaderKind.Content),
			HeaderInfo.CreateSingle<long>("Content-Length", Parser.Long.TryParse, HttpHeaderKind.Content),
			HeaderInfo.CreateSingle<Uri>("Content-Location", Parser.Uri.TryParse, HttpHeaderKind.Content),
			HeaderInfo.CreateSingle<byte[]>("Content-MD5", Parser.MD5.TryParse, HttpHeaderKind.Content),
			HeaderInfo.CreateSingle<ContentRangeHeaderValue>("Content-Range", ContentRangeHeaderValue.TryParse, HttpHeaderKind.Content),
			HeaderInfo.CreateSingle<MediaTypeHeaderValue>("Content-Type", MediaTypeHeaderValue.TryParse, HttpHeaderKind.Content),
			HeaderInfo.CreateSingle<DateTimeOffset>("Date", Parser.DateTime.TryParse, HttpHeaderKind.Request | HttpHeaderKind.Response, Parser.DateTime.ToString),
			HeaderInfo.CreateSingle<EntityTagHeaderValue>("ETag", EntityTagHeaderValue.TryParse, HttpHeaderKind.Response),
			HeaderInfo.CreateMulti<NameValueWithParametersHeaderValue>("Expect", NameValueWithParametersHeaderValue.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateSingle<DateTimeOffset>("Expires", Parser.DateTime.TryParse, HttpHeaderKind.Content, Parser.DateTime.ToString),
			HeaderInfo.CreateSingle<string>("From", Parser.EmailAddress.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateSingle<string>("Host", Parser.Host.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateMulti<EntityTagHeaderValue>("If-Match", EntityTagHeaderValue.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateSingle<DateTimeOffset>("If-Modified-Since", Parser.DateTime.TryParse, HttpHeaderKind.Request, Parser.DateTime.ToString),
			HeaderInfo.CreateMulti<EntityTagHeaderValue>("If-None-Match", EntityTagHeaderValue.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateSingle<RangeConditionHeaderValue>("If-Range", RangeConditionHeaderValue.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateSingle<DateTimeOffset>("If-Unmodified-Since", Parser.DateTime.TryParse, HttpHeaderKind.Request, Parser.DateTime.ToString),
			HeaderInfo.CreateSingle<DateTimeOffset>("Last-Modified", Parser.DateTime.TryParse, HttpHeaderKind.Content, Parser.DateTime.ToString),
			HeaderInfo.CreateSingle<Uri>("Location", Parser.Uri.TryParse, HttpHeaderKind.Response),
			HeaderInfo.CreateSingle<int>("Max-Forwards", Parser.Int.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateMulti<NameValueHeaderValue>("Pragma", NameValueHeaderValue.TryParsePragma, HttpHeaderKind.Request | HttpHeaderKind.Response),
			HeaderInfo.CreateMulti<AuthenticationHeaderValue>("Proxy-Authenticate", AuthenticationHeaderValue.TryParse, HttpHeaderKind.Response),
			HeaderInfo.CreateSingle<AuthenticationHeaderValue>("Proxy-Authorization", AuthenticationHeaderValue.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateSingle<RangeHeaderValue>("Range", RangeHeaderValue.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateSingle<Uri>("Referer", Parser.Uri.TryParse, HttpHeaderKind.Request),
			HeaderInfo.CreateSingle<RetryConditionHeaderValue>("Retry-After", RetryConditionHeaderValue.TryParse, HttpHeaderKind.Response),
			HeaderInfo.CreateMulti<ProductInfoHeaderValue>("Server", ProductInfoHeaderValue.TryParse, HttpHeaderKind.Response, 1, " "),
			HeaderInfo.CreateMulti<TransferCodingWithQualityHeaderValue>("TE", TransferCodingWithQualityHeaderValue.TryParse, HttpHeaderKind.Request, 0),
			HeaderInfo.CreateMulti<string>("Trailer", CollectionParser.TryParse, HttpHeaderKind.Request | HttpHeaderKind.Response),
			HeaderInfo.CreateMulti<TransferCodingHeaderValue>("Transfer-Encoding", TransferCodingHeaderValue.TryParse, HttpHeaderKind.Request | HttpHeaderKind.Response),
			HeaderInfo.CreateMulti<ProductHeaderValue>("Upgrade", ProductHeaderValue.TryParse, HttpHeaderKind.Request | HttpHeaderKind.Response),
			HeaderInfo.CreateMulti<ProductInfoHeaderValue>("User-Agent", ProductInfoHeaderValue.TryParse, HttpHeaderKind.Request, 1, " "),
			HeaderInfo.CreateMulti<string>("Vary", CollectionParser.TryParse, HttpHeaderKind.Response),
			HeaderInfo.CreateMulti<ViaHeaderValue>("Via", ViaHeaderValue.TryParse, HttpHeaderKind.Request | HttpHeaderKind.Response),
			HeaderInfo.CreateMulti<WarningHeaderValue>("Warning", WarningHeaderValue.TryParse, HttpHeaderKind.Request | HttpHeaderKind.Response),
			HeaderInfo.CreateMulti<AuthenticationHeaderValue>("WWW-Authenticate", AuthenticationHeaderValue.TryParse, HttpHeaderKind.Response)
		};
		known_headers = new Dictionary<string, HeaderInfo>(StringComparer.OrdinalIgnoreCase);
		HeaderInfo[] array = obj;
		foreach (HeaderInfo headerInfo in array)
		{
			known_headers.Add(headerInfo.Name, headerInfo);
		}
	}

	protected HttpHeaders()
	{
		headers = new Dictionary<string, HeaderBucket>(StringComparer.OrdinalIgnoreCase);
	}

	internal HttpHeaders(HttpHeaderKind headerKind)
		: this()
	{
		HeaderKind = headerKind;
	}

	public void Add(string name, string value)
	{
		Add(name, new string[1] { value });
	}

	public void Add(string name, IEnumerable<string> values)
	{
		if (values == null)
		{
			throw new ArgumentNullException("values");
		}
		AddInternal(name, values, CheckName(name), ignoreInvalid: false);
	}

	internal bool AddValue(string value, HeaderInfo headerInfo, bool ignoreInvalid)
	{
		return AddInternal(headerInfo.Name, new string[1] { value }, headerInfo, ignoreInvalid);
	}

	private bool AddInternal(string name, IEnumerable<string> values, HeaderInfo headerInfo, bool ignoreInvalid)
	{
		headers.TryGetValue(name, out var value);
		bool result = true;
		foreach (string value2 in values)
		{
			bool flag = value == null;
			if (headerInfo != null)
			{
				if (!headerInfo.TryParse(value2, out var result2))
				{
					if (ignoreInvalid)
					{
						result = false;
						continue;
					}
					throw new FormatException();
				}
				if (headerInfo.AllowsMany)
				{
					if (value == null)
					{
						value = new HeaderBucket(headerInfo.CreateCollection(this), headerInfo.CustomToString);
					}
					headerInfo.AddToCollection(value.Parsed, result2);
				}
				else
				{
					if (value != null)
					{
						throw new FormatException();
					}
					value = new HeaderBucket(result2, headerInfo.CustomToString);
				}
			}
			else
			{
				if (value == null)
				{
					value = new HeaderBucket(null, null);
				}
				value.Values.Add(value2 ?? string.Empty);
			}
			if (flag)
			{
				headers.Add(name, value);
			}
		}
		return result;
	}

	public bool TryAddWithoutValidation(string name, string value)
	{
		return TryAddWithoutValidation(name, new string[1] { value });
	}

	public bool TryAddWithoutValidation(string name, IEnumerable<string> values)
	{
		if (values == null)
		{
			throw new ArgumentNullException("values");
		}
		if (!TryCheckName(name, out var _))
		{
			return false;
		}
		AddInternal(name, values, null, ignoreInvalid: true);
		return true;
	}

	private HeaderInfo CheckName(string name)
	{
		if (string.IsNullOrEmpty(name))
		{
			throw new ArgumentException("name");
		}
		Parser.Token.Check(name);
		if (known_headers.TryGetValue(name, out var value) && (value.HeaderKind & HeaderKind) == 0)
		{
			if (HeaderKind != 0 && ((HeaderKind | value.HeaderKind) & HttpHeaderKind.Content) != 0)
			{
				throw new InvalidOperationException(name);
			}
			return null;
		}
		return value;
	}

	private bool TryCheckName(string name, out HeaderInfo headerInfo)
	{
		if (!Parser.Token.TryCheck(name))
		{
			headerInfo = null;
			return false;
		}
		if (known_headers.TryGetValue(name, out headerInfo) && (headerInfo.HeaderKind & HeaderKind) == 0 && HeaderKind != 0 && ((HeaderKind | headerInfo.HeaderKind) & HttpHeaderKind.Content) != 0)
		{
			return false;
		}
		return true;
	}

	public void Clear()
	{
		connectionclose = null;
		transferEncodingChunked = null;
		headers.Clear();
	}

	public bool Contains(string name)
	{
		CheckName(name);
		return headers.ContainsKey(name);
	}

	public IEnumerator<KeyValuePair<string, IEnumerable<string>>> GetEnumerator()
	{
		foreach (KeyValuePair<string, HeaderBucket> header in headers)
		{
			HeaderBucket bucket = headers[header.Key];
			known_headers.TryGetValue(header.Key, out var value);
			List<string> allHeaderValues = GetAllHeaderValues(bucket, value);
			if (allHeaderValues != null)
			{
				yield return new KeyValuePair<string, IEnumerable<string>>(header.Key, allHeaderValues);
			}
		}
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		return GetEnumerator();
	}

	public IEnumerable<string> GetValues(string name)
	{
		CheckName(name);
		if (!TryGetValues(name, out var values))
		{
			throw new InvalidOperationException();
		}
		return values;
	}

	public bool Remove(string name)
	{
		CheckName(name);
		return headers.Remove(name);
	}

	public bool TryGetValues(string name, out IEnumerable<string> values)
	{
		if (!TryCheckName(name, out var headerInfo))
		{
			values = null;
			return false;
		}
		if (!headers.TryGetValue(name, out var value))
		{
			values = null;
			return false;
		}
		values = GetAllHeaderValues(value, headerInfo);
		return true;
	}

	internal static string GetSingleHeaderString(string key, IEnumerable<string> values)
	{
		string text = ",";
		if (known_headers.TryGetValue(key, out var value) && value.AllowsMany)
		{
			text = value.Separator;
		}
		StringBuilder stringBuilder = new StringBuilder();
		bool flag = true;
		foreach (string value2 in values)
		{
			if (!flag)
			{
				stringBuilder.Append(text);
				if (text != " ")
				{
					stringBuilder.Append(" ");
				}
			}
			stringBuilder.Append(value2);
			flag = false;
		}
		if (flag)
		{
			return null;
		}
		return stringBuilder.ToString();
	}

	public override string ToString()
	{
		StringBuilder stringBuilder = new StringBuilder();
		using (IEnumerator<KeyValuePair<string, IEnumerable<string>>> enumerator = GetEnumerator())
		{
			while (enumerator.MoveNext())
			{
				KeyValuePair<string, IEnumerable<string>> current = enumerator.Current;
				stringBuilder.Append(current.Key);
				stringBuilder.Append(": ");
				stringBuilder.Append(GetSingleHeaderString(current.Key, current.Value));
				stringBuilder.Append("\r\n");
			}
		}
		return stringBuilder.ToString();
	}

	internal void AddOrRemove(string name, string value)
	{
		if (string.IsNullOrEmpty(value))
		{
			Remove(name);
		}
		else
		{
			SetValue(name, value);
		}
	}

	internal void AddOrRemove<T>(string name, T value, Func<object, string> converter = null) where T : class
	{
		if (value == null)
		{
			Remove(name);
		}
		else
		{
			SetValue(name, value, converter);
		}
	}

	internal void AddOrRemove<T>(string name, T? value) where T : struct
	{
		AddOrRemove(name, value, null);
	}

	internal void AddOrRemove<T>(string name, T? value, Func<object, string> converter) where T : struct
	{
		if (!value.HasValue)
		{
			Remove(name);
		}
		else
		{
			SetValue(name, value, converter);
		}
	}

	private List<string> GetAllHeaderValues(HeaderBucket bucket, HeaderInfo headerInfo)
	{
		List<string> list = null;
		if (headerInfo != null && headerInfo.AllowsMany)
		{
			list = headerInfo.ToStringCollection(bucket.Parsed);
		}
		else if (bucket.Parsed != null)
		{
			string text = bucket.ParsedToString();
			if (!string.IsNullOrEmpty(text))
			{
				list = new List<string>();
				list.Add(text);
			}
		}
		if (bucket.HasStringValues)
		{
			if (list == null)
			{
				list = new List<string>();
			}
			list.AddRange(bucket.Values);
		}
		return list;
	}

	internal static HttpHeaderKind GetKnownHeaderKind(string name)
	{
		if (string.IsNullOrEmpty(name))
		{
			throw new ArgumentException("name");
		}
		if (known_headers.TryGetValue(name, out var value))
		{
			return value.HeaderKind;
		}
		return HttpHeaderKind.None;
	}

	internal T GetValue<T>(string name)
	{
		if (!headers.TryGetValue(name, out var value))
		{
			return default(T);
		}
		if (value.HasStringValues)
		{
			if (!known_headers[name].TryParse(value.Values[0], out var result))
			{
				if (!(typeof(T) == typeof(string)))
				{
					return default(T);
				}
				return (T)(object)value.Values[0];
			}
			value.Parsed = result;
			value.Values = null;
		}
		return (T)value.Parsed;
	}

	internal HttpHeaderValueCollection<T> GetValues<T>(string name) where T : class
	{
		if (!headers.TryGetValue(name, out var value))
		{
			HeaderInfo headerInfo = known_headers[name];
			value = new HeaderBucket(new HttpHeaderValueCollection<T>(this, headerInfo), headerInfo.CustomToString);
			headers.Add(name, value);
		}
		HttpHeaderValueCollection<T> httpHeaderValueCollection = (HttpHeaderValueCollection<T>)value.Parsed;
		if (value.HasStringValues)
		{
			HeaderInfo headerInfo2 = known_headers[name];
			if (httpHeaderValueCollection == null)
			{
				httpHeaderValueCollection = (HttpHeaderValueCollection<T>)(value.Parsed = new HttpHeaderValueCollection<T>(this, headerInfo2));
			}
			for (int i = 0; i < value.Values.Count; i++)
			{
				string text = value.Values[i];
				if (!headerInfo2.TryParse(text, out var result))
				{
					httpHeaderValueCollection.AddInvalidValue(text);
				}
				else
				{
					headerInfo2.AddToCollection(httpHeaderValueCollection, result);
				}
			}
			value.Values.Clear();
		}
		return httpHeaderValueCollection;
	}

	internal void SetValue<T>(string name, T value, Func<object, string> toStringConverter = null)
	{
		headers[name] = new HeaderBucket(value, toStringConverter);
	}
}
