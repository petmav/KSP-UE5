using System.IO;
using System.Net.Http.Headers;
using System.Threading;
using System.Threading.Tasks;

namespace System.Net.Http;

public class HttpClient : HttpMessageInvoker
{
	private static readonly TimeSpan TimeoutDefault = TimeSpan.FromSeconds(100.0);

	private Uri base_address;

	private CancellationTokenSource cts;

	private bool disposed;

	private HttpRequestHeaders headers;

	private long buffer_size;

	private TimeSpan timeout;

	public Uri BaseAddress
	{
		get
		{
			return base_address;
		}
		set
		{
			base_address = value;
		}
	}

	public HttpRequestHeaders DefaultRequestHeaders => headers ?? (headers = new HttpRequestHeaders());

	public long MaxResponseContentBufferSize
	{
		get
		{
			return buffer_size;
		}
		set
		{
			if (value <= 0)
			{
				throw new ArgumentOutOfRangeException();
			}
			buffer_size = value;
		}
	}

	public TimeSpan Timeout
	{
		get
		{
			return timeout;
		}
		set
		{
			if (value != System.Threading.Timeout.InfiniteTimeSpan && (value <= TimeSpan.Zero || value.TotalMilliseconds > 2147483647.0))
			{
				throw new ArgumentOutOfRangeException();
			}
			timeout = value;
		}
	}

	public HttpClient()
		: this(new HttpClientHandler(), disposeHandler: true)
	{
	}

	public HttpClient(HttpMessageHandler handler)
		: this(handler, disposeHandler: true)
	{
	}

	public HttpClient(HttpMessageHandler handler, bool disposeHandler)
		: base(handler, disposeHandler)
	{
		buffer_size = 2147483647L;
		timeout = TimeoutDefault;
		cts = new CancellationTokenSource();
	}

	public void CancelPendingRequests()
	{
		using CancellationTokenSource cancellationTokenSource = Interlocked.Exchange(ref cts, new CancellationTokenSource());
		cancellationTokenSource.Cancel();
	}

	protected override void Dispose(bool disposing)
	{
		if (disposing && !disposed)
		{
			disposed = true;
			cts.Dispose();
		}
		base.Dispose(disposing);
	}

	public Task<HttpResponseMessage> DeleteAsync(string requestUri)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Delete, requestUri));
	}

	public Task<HttpResponseMessage> DeleteAsync(string requestUri, CancellationToken cancellationToken)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Delete, requestUri), cancellationToken);
	}

	public Task<HttpResponseMessage> DeleteAsync(Uri requestUri)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Delete, requestUri));
	}

	public Task<HttpResponseMessage> DeleteAsync(Uri requestUri, CancellationToken cancellationToken)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Delete, requestUri), cancellationToken);
	}

	public Task<HttpResponseMessage> GetAsync(string requestUri)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Get, requestUri));
	}

	public Task<HttpResponseMessage> GetAsync(string requestUri, CancellationToken cancellationToken)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Get, requestUri), cancellationToken);
	}

	public Task<HttpResponseMessage> GetAsync(string requestUri, HttpCompletionOption completionOption)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Get, requestUri), completionOption);
	}

	public Task<HttpResponseMessage> GetAsync(string requestUri, HttpCompletionOption completionOption, CancellationToken cancellationToken)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Get, requestUri), completionOption, cancellationToken);
	}

	public Task<HttpResponseMessage> GetAsync(Uri requestUri)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Get, requestUri));
	}

	public Task<HttpResponseMessage> GetAsync(Uri requestUri, CancellationToken cancellationToken)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Get, requestUri), cancellationToken);
	}

	public Task<HttpResponseMessage> GetAsync(Uri requestUri, HttpCompletionOption completionOption)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Get, requestUri), completionOption);
	}

	public Task<HttpResponseMessage> GetAsync(Uri requestUri, HttpCompletionOption completionOption, CancellationToken cancellationToken)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Get, requestUri), completionOption, cancellationToken);
	}

	public Task<HttpResponseMessage> PostAsync(string requestUri, HttpContent content)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Post, requestUri)
		{
			Content = content
		});
	}

	public Task<HttpResponseMessage> PostAsync(string requestUri, HttpContent content, CancellationToken cancellationToken)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Post, requestUri)
		{
			Content = content
		}, cancellationToken);
	}

	public Task<HttpResponseMessage> PostAsync(Uri requestUri, HttpContent content)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Post, requestUri)
		{
			Content = content
		});
	}

	public Task<HttpResponseMessage> PostAsync(Uri requestUri, HttpContent content, CancellationToken cancellationToken)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Post, requestUri)
		{
			Content = content
		}, cancellationToken);
	}

	public Task<HttpResponseMessage> PutAsync(Uri requestUri, HttpContent content)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Put, requestUri)
		{
			Content = content
		});
	}

	public Task<HttpResponseMessage> PutAsync(Uri requestUri, HttpContent content, CancellationToken cancellationToken)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Put, requestUri)
		{
			Content = content
		}, cancellationToken);
	}

	public Task<HttpResponseMessage> PutAsync(string requestUri, HttpContent content)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Put, requestUri)
		{
			Content = content
		});
	}

	public Task<HttpResponseMessage> PutAsync(string requestUri, HttpContent content, CancellationToken cancellationToken)
	{
		return SendAsync(new HttpRequestMessage(HttpMethod.Put, requestUri)
		{
			Content = content
		}, cancellationToken);
	}

	public Task<HttpResponseMessage> SendAsync(HttpRequestMessage request)
	{
		return SendAsync(request, HttpCompletionOption.ResponseContentRead, CancellationToken.None);
	}

	public Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, HttpCompletionOption completionOption)
	{
		return SendAsync(request, completionOption, CancellationToken.None);
	}

	public override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
	{
		return SendAsync(request, HttpCompletionOption.ResponseContentRead, cancellationToken);
	}

	public Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, HttpCompletionOption completionOption, CancellationToken cancellationToken)
	{
		if (request == null)
		{
			throw new ArgumentNullException("request");
		}
		if (request.SetIsUsed())
		{
			throw new InvalidOperationException("Cannot send the same request message multiple times");
		}
		Uri requestUri = request.RequestUri;
		if (requestUri == null)
		{
			if (base_address == null)
			{
				throw new InvalidOperationException("The request URI must either be an absolute URI or BaseAddress must be set");
			}
			request.RequestUri = base_address;
		}
		else if (!requestUri.IsAbsoluteUri || (requestUri.Scheme == Uri.UriSchemeFile && requestUri.OriginalString.StartsWith("/", StringComparison.Ordinal)))
		{
			if (base_address == null)
			{
				throw new InvalidOperationException("The request URI must either be an absolute URI or BaseAddress must be set");
			}
			request.RequestUri = new Uri(base_address, requestUri);
		}
		if (headers != null)
		{
			request.Headers.AddHeaders(headers);
		}
		return SendAsyncWorker(request, completionOption, cancellationToken);
	}

	private async Task<HttpResponseMessage> SendAsyncWorker(HttpRequestMessage request, HttpCompletionOption completionOption, CancellationToken cancellationToken)
	{
		using CancellationTokenSource lcts = CancellationTokenSource.CreateLinkedTokenSource(cts.Token, cancellationToken);
		lcts.CancelAfter(timeout);
		HttpResponseMessage response = await (base.SendAsync(request, lcts.Token) ?? throw new InvalidOperationException("Handler failed to return a value")).ConfigureAwait(continueOnCapturedContext: false);
		if (response == null)
		{
			throw new InvalidOperationException("Handler failed to return a response");
		}
		if (response.Content != null && (completionOption & HttpCompletionOption.ResponseHeadersRead) == 0)
		{
			await response.Content.LoadIntoBufferAsync(MaxResponseContentBufferSize).ConfigureAwait(continueOnCapturedContext: false);
		}
		return response;
	}

	public async Task<byte[]> GetByteArrayAsync(string requestUri)
	{
		using HttpResponseMessage resp = await GetAsync(requestUri, HttpCompletionOption.ResponseContentRead).ConfigureAwait(continueOnCapturedContext: false);
		resp.EnsureSuccessStatusCode();
		return await resp.Content.ReadAsByteArrayAsync().ConfigureAwait(continueOnCapturedContext: false);
	}

	public async Task<byte[]> GetByteArrayAsync(Uri requestUri)
	{
		using HttpResponseMessage resp = await GetAsync(requestUri, HttpCompletionOption.ResponseContentRead).ConfigureAwait(continueOnCapturedContext: false);
		resp.EnsureSuccessStatusCode();
		return await resp.Content.ReadAsByteArrayAsync().ConfigureAwait(continueOnCapturedContext: false);
	}

	public async Task<Stream> GetStreamAsync(string requestUri)
	{
		HttpResponseMessage obj = await GetAsync(requestUri, HttpCompletionOption.ResponseHeadersRead).ConfigureAwait(continueOnCapturedContext: false);
		obj.EnsureSuccessStatusCode();
		return await obj.Content.ReadAsStreamAsync().ConfigureAwait(continueOnCapturedContext: false);
	}

	public async Task<Stream> GetStreamAsync(Uri requestUri)
	{
		HttpResponseMessage obj = await GetAsync(requestUri, HttpCompletionOption.ResponseHeadersRead).ConfigureAwait(continueOnCapturedContext: false);
		obj.EnsureSuccessStatusCode();
		return await obj.Content.ReadAsStreamAsync().ConfigureAwait(continueOnCapturedContext: false);
	}

	public async Task<string> GetStringAsync(string requestUri)
	{
		using HttpResponseMessage resp = await GetAsync(requestUri, HttpCompletionOption.ResponseContentRead).ConfigureAwait(continueOnCapturedContext: false);
		resp.EnsureSuccessStatusCode();
		return await resp.Content.ReadAsStringAsync().ConfigureAwait(continueOnCapturedContext: false);
	}

	public async Task<string> GetStringAsync(Uri requestUri)
	{
		using HttpResponseMessage resp = await GetAsync(requestUri, HttpCompletionOption.ResponseContentRead).ConfigureAwait(continueOnCapturedContext: false);
		resp.EnsureSuccessStatusCode();
		return await resp.Content.ReadAsStringAsync().ConfigureAwait(continueOnCapturedContext: false);
	}
}
