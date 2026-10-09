using System.Net.Http.Headers;

namespace System.Net.Http;

public class HttpMethod : IEquatable<HttpMethod>
{
	private static readonly HttpMethod delete_method = new HttpMethod("DELETE");

	private static readonly HttpMethod get_method = new HttpMethod("GET");

	private static readonly HttpMethod head_method = new HttpMethod("HEAD");

	private static readonly HttpMethod options_method = new HttpMethod("OPTIONS");

	private static readonly HttpMethod post_method = new HttpMethod("POST");

	private static readonly HttpMethod put_method = new HttpMethod("PUT");

	private static readonly HttpMethod trace_method = new HttpMethod("TRACE");

	private readonly string method;

	public static HttpMethod Delete => delete_method;

	public static HttpMethod Get => get_method;

	public static HttpMethod Head => head_method;

	public string Method => method;

	public static HttpMethod Options => options_method;

	public static HttpMethod Post => post_method;

	public static HttpMethod Put => put_method;

	public static HttpMethod Trace => trace_method;

	public HttpMethod(string method)
	{
		if (string.IsNullOrEmpty(method))
		{
			throw new ArgumentException("method");
		}
		Parser.Token.Check(method);
		this.method = method;
	}

	public static bool operator ==(HttpMethod left, HttpMethod right)
	{
		if ((object)left == null || (object)right == null)
		{
			return (object)left == right;
		}
		return left.Equals(right);
	}

	public static bool operator !=(HttpMethod left, HttpMethod right)
	{
		return !(left == right);
	}

	public bool Equals(HttpMethod other)
	{
		return string.Equals(method, other.method, StringComparison.OrdinalIgnoreCase);
	}

	public override bool Equals(object obj)
	{
		if (obj is HttpMethod other)
		{
			return Equals(other);
		}
		return false;
	}

	public override int GetHashCode()
	{
		return method.GetHashCode();
	}

	public override string ToString()
	{
		return method;
	}
}
