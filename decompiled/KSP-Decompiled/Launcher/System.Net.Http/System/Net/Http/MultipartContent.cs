using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net.Http.Headers;
using System.Text;
using System.Threading.Tasks;

namespace System.Net.Http;

public class MultipartContent : HttpContent, IEnumerable<HttpContent>, IEnumerable
{
	private List<HttpContent> nested_content;

	private readonly string boundary;

	public MultipartContent()
		: this("mixed")
	{
	}

	public MultipartContent(string subtype)
		: this(subtype, Guid.NewGuid().ToString("D", CultureInfo.InvariantCulture))
	{
	}

	public MultipartContent(string subtype, string boundary)
	{
		if (string.IsNullOrWhiteSpace(subtype))
		{
			throw new ArgumentException("boundary");
		}
		if (string.IsNullOrWhiteSpace(boundary))
		{
			throw new ArgumentException("boundary");
		}
		if (boundary.Length > 70)
		{
			throw new ArgumentOutOfRangeException("boundary");
		}
		if (boundary.Last() == ' ' || !IsValidRFC2049(boundary))
		{
			throw new ArgumentException("boundary");
		}
		this.boundary = boundary;
		nested_content = new List<HttpContent>(2);
		base.Headers.ContentType = new MediaTypeHeaderValue("multipart/" + subtype)
		{
			Parameters = 
			{
				new NameValueHeaderValue("boundary", "\"" + boundary + "\"")
			}
		};
	}

	private static bool IsValidRFC2049(string s)
	{
		foreach (char c in s)
		{
			if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9'))
			{
				switch (c)
				{
				case '\'':
				case '(':
				case ')':
				case '+':
				case ',':
				case '-':
				case '.':
				case '/':
				case ':':
				case '=':
				case '?':
					continue;
				}
				return false;
			}
		}
		return true;
	}

	public virtual void Add(HttpContent content)
	{
		if (content == null)
		{
			throw new ArgumentNullException("content");
		}
		if (nested_content == null)
		{
			nested_content = new List<HttpContent>();
		}
		nested_content.Add(content);
	}

	protected override void Dispose(bool disposing)
	{
		if (disposing)
		{
			foreach (HttpContent item in nested_content)
			{
				item.Dispose();
			}
			nested_content = null;
		}
		base.Dispose(disposing);
	}

	protected internal override async Task SerializeToStreamAsync(Stream stream, TransportContext context)
	{
		StringBuilder sb = new StringBuilder();
		sb.Append('-').Append('-');
		sb.Append(boundary);
		sb.Append('\r').Append('\n');
		byte[] bytes;
		for (int i = 0; i < nested_content.Count; i++)
		{
			HttpContent c = nested_content[i];
			foreach (KeyValuePair<string, IEnumerable<string>> header in c.Headers)
			{
				sb.Append(header.Key);
				sb.Append(':').Append(' ');
				foreach (string item in header.Value)
				{
					sb.Append(item);
				}
				sb.Append('\r').Append('\n');
			}
			sb.Append('\r').Append('\n');
			bytes = Encoding.ASCII.GetBytes(sb.ToString());
			sb.Clear();
			await stream.WriteAsync(bytes, 0, bytes.Length).ConfigureAwait(continueOnCapturedContext: false);
			await c.SerializeToStreamAsync(stream, context).ConfigureAwait(continueOnCapturedContext: false);
			if (i != nested_content.Count - 1)
			{
				sb.Append('\r').Append('\n');
				sb.Append('-').Append('-');
				sb.Append(boundary);
				sb.Append('\r').Append('\n');
			}
		}
		sb.Append('\r').Append('\n');
		sb.Append('-').Append('-');
		sb.Append(boundary);
		sb.Append('-').Append('-');
		sb.Append('\r').Append('\n');
		bytes = Encoding.ASCII.GetBytes(sb.ToString());
		await stream.WriteAsync(bytes, 0, bytes.Length).ConfigureAwait(continueOnCapturedContext: false);
	}

	protected internal override bool TryComputeLength(out long length)
	{
		length = 12 + 2 * boundary.Length;
		for (int i = 0; i < nested_content.Count; i++)
		{
			HttpContent httpContent = nested_content[i];
			foreach (KeyValuePair<string, IEnumerable<string>> header in httpContent.Headers)
			{
				length += header.Key.Length;
				length += 4L;
				foreach (string item in header.Value)
				{
					length += item.Length;
				}
			}
			if (!httpContent.TryComputeLength(out var length2))
			{
				return false;
			}
			length += 2L;
			length += length2;
			if (i != nested_content.Count - 1)
			{
				length += 6L;
				length += boundary.Length;
			}
		}
		return true;
	}

	public IEnumerator<HttpContent> GetEnumerator()
	{
		return nested_content.GetEnumerator();
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		return nested_content.GetEnumerator();
	}
}
