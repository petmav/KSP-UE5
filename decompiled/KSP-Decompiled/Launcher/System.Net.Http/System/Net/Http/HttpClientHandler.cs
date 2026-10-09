using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Http.Headers;
using System.Net.Security;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using System.Threading.Tasks;

namespace System.Net.Http;

public class HttpClientHandler : HttpMessageHandler
{
	private static long groupCounter;

	private bool allowAutoRedirect;

	private DecompressionMethods automaticDecompression;

	private CookieContainer cookieContainer;

	private ICredentials credentials;

	private int maxAutomaticRedirections;

	private long maxRequestContentBufferSize;

	private bool preAuthenticate;

	private IWebProxy proxy;

	private bool useCookies;

	private bool useDefaultCredentials;

	private bool useProxy;

	private ClientCertificateOption certificate;

	private bool sentRequest;

	private string connectionGroupName;

	private bool disposed;

	public bool AllowAutoRedirect
	{
		get
		{
			return allowAutoRedirect;
		}
		set
		{
			EnsureModifiability();
			allowAutoRedirect = value;
		}
	}

	public DecompressionMethods AutomaticDecompression
	{
		get
		{
			return automaticDecompression;
		}
		set
		{
			EnsureModifiability();
			automaticDecompression = value;
		}
	}

	public ClientCertificateOption ClientCertificateOptions
	{
		get
		{
			return certificate;
		}
		set
		{
			EnsureModifiability();
			certificate = value;
		}
	}

	public CookieContainer CookieContainer
	{
		get
		{
			return cookieContainer ?? (cookieContainer = new CookieContainer());
		}
		set
		{
			EnsureModifiability();
			cookieContainer = value;
		}
	}

	public ICredentials Credentials
	{
		get
		{
			return credentials;
		}
		set
		{
			EnsureModifiability();
			credentials = value;
		}
	}

	public int MaxAutomaticRedirections
	{
		get
		{
			return maxAutomaticRedirections;
		}
		set
		{
			EnsureModifiability();
			if (value <= 0)
			{
				throw new ArgumentOutOfRangeException();
			}
			maxAutomaticRedirections = value;
		}
	}

	public long MaxRequestContentBufferSize
	{
		get
		{
			return maxRequestContentBufferSize;
		}
		set
		{
			EnsureModifiability();
			if (value < 0)
			{
				throw new ArgumentOutOfRangeException();
			}
			maxRequestContentBufferSize = value;
		}
	}

	public bool PreAuthenticate
	{
		get
		{
			return preAuthenticate;
		}
		set
		{
			EnsureModifiability();
			preAuthenticate = value;
		}
	}

	public IWebProxy Proxy
	{
		get
		{
			return proxy;
		}
		set
		{
			EnsureModifiability();
			if (!UseProxy)
			{
				throw new InvalidOperationException();
			}
			proxy = value;
		}
	}

	public virtual bool SupportsAutomaticDecompression => true;

	public virtual bool SupportsProxy => true;

	public virtual bool SupportsRedirectConfiguration => true;

	public bool UseCookies
	{
		get
		{
			return useCookies;
		}
		set
		{
			EnsureModifiability();
			useCookies = value;
		}
	}

	public bool UseDefaultCredentials
	{
		get
		{
			return useDefaultCredentials;
		}
		set
		{
			EnsureModifiability();
			useDefaultCredentials = value;
		}
	}

	public bool UseProxy
	{
		get
		{
			return useProxy;
		}
		set
		{
			EnsureModifiability();
			useProxy = value;
		}
	}

	public bool CheckCertificateRevocationList
	{
		get
		{
			throw new NotImplementedException();
		}
		set
		{
			throw new NotImplementedException();
		}
	}

	public X509CertificateCollection ClientCertificates
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public ICredentials DefaultProxyCredentials
	{
		get
		{
			throw new NotImplementedException();
		}
		set
		{
			throw new NotImplementedException();
		}
	}

	public int MaxConnectionsPerServer
	{
		get
		{
			throw new NotImplementedException();
		}
		set
		{
			throw new NotImplementedException();
		}
	}

	public int MaxResponseHeadersLength
	{
		get
		{
			throw new NotImplementedException();
		}
		set
		{
			throw new NotImplementedException();
		}
	}

	public IDictionary<string, object> Properties
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public Func<HttpRequestMessage, X509Certificate2, X509Chain, SslPolicyErrors, bool> ServerCertificateCustomValidationCallback
	{
		get
		{
			throw new NotImplementedException();
		}
		set
		{
			throw new NotImplementedException();
		}
	}

	public SslProtocols SslProtocols
	{
		get
		{
			throw new NotImplementedException();
		}
		set
		{
			throw new NotImplementedException();
		}
	}

	public HttpClientHandler()
	{
		allowAutoRedirect = true;
		maxAutomaticRedirections = 50;
		maxRequestContentBufferSize = 2147483647L;
		useCookies = true;
		useProxy = true;
		connectionGroupName = "HttpClientHandler" + Interlocked.Increment(ref groupCounter);
	}

	internal void EnsureModifiability()
	{
		if (sentRequest)
		{
			throw new InvalidOperationException("This instance has already started one or more requests. Properties can only be modified before sending the first request.");
		}
	}

	protected override void Dispose(bool disposing)
	{
		if (disposing && !disposed)
		{
			Volatile.Write(ref disposed, value: true);
			ServicePointManager.CloseConnectionGroup(connectionGroupName);
		}
		base.Dispose(disposing);
	}

	internal virtual HttpWebRequest CreateWebRequest(HttpRequestMessage request)
	{
		HttpWebRequest httpWebRequest = new HttpWebRequest(request.RequestUri);
		httpWebRequest.ThrowOnError = false;
		httpWebRequest.AllowWriteStreamBuffering = false;
		httpWebRequest.ConnectionGroupName = connectionGroupName;
		httpWebRequest.Method = request.Method.Method;
		httpWebRequest.ProtocolVersion = request.Version;
		if (httpWebRequest.ProtocolVersion == HttpVersion.Version10)
		{
			httpWebRequest.KeepAlive = request.Headers.ConnectionKeepAlive;
		}
		else
		{
			httpWebRequest.KeepAlive = request.Headers.ConnectionClose != true;
		}
		if (allowAutoRedirect)
		{
			httpWebRequest.AllowAutoRedirect = true;
			httpWebRequest.MaximumAutomaticRedirections = maxAutomaticRedirections;
		}
		else
		{
			httpWebRequest.AllowAutoRedirect = false;
		}
		httpWebRequest.AutomaticDecompression = automaticDecompression;
		httpWebRequest.PreAuthenticate = preAuthenticate;
		if (useCookies)
		{
			httpWebRequest.CookieContainer = CookieContainer;
		}
		if (useDefaultCredentials)
		{
			httpWebRequest.UseDefaultCredentials = true;
		}
		else
		{
			httpWebRequest.Credentials = credentials;
		}
		if (useProxy)
		{
			httpWebRequest.Proxy = proxy;
		}
		else
		{
			httpWebRequest.Proxy = null;
		}
		httpWebRequest.ServicePoint.Expect100Continue = request.Headers.ExpectContinue == true;
		WebHeaderCollection headers = httpWebRequest.Headers;
		foreach (KeyValuePair<string, IEnumerable<string>> header in request.Headers)
		{
			IEnumerable<string> enumerable = header.Value;
			if (header.Key == "Host")
			{
				httpWebRequest.Host = request.Headers.Host;
				continue;
			}
			if (header.Key == "Transfer-Encoding")
			{
				enumerable = enumerable.Where((string l) => l != "chunked");
			}
			string singleHeaderString = HttpHeaders.GetSingleHeaderString(header.Key, enumerable);
			if (singleHeaderString != null)
			{
				headers.AddInternal(header.Key, singleHeaderString);
			}
		}
		return httpWebRequest;
	}

	private HttpResponseMessage CreateResponseMessage(HttpWebResponse wr, HttpRequestMessage requestMessage, CancellationToken cancellationToken)
	{
		HttpResponseMessage httpResponseMessage = new HttpResponseMessage(wr.StatusCode);
		httpResponseMessage.RequestMessage = requestMessage;
		httpResponseMessage.ReasonPhrase = wr.StatusDescription;
		httpResponseMessage.Content = new StreamContent(wr.GetResponseStream(), cancellationToken);
		WebHeaderCollection headers = wr.Headers;
		for (int i = 0; i < headers.Count; i++)
		{
			string key = headers.GetKey(i);
			string[] values = headers.GetValues(i);
			HttpHeaders httpHeaders = ((HttpHeaders.GetKnownHeaderKind(key) != HttpHeaderKind.Content) ? ((HttpHeaders)httpResponseMessage.Headers) : ((HttpHeaders)httpResponseMessage.Content.Headers));
			httpHeaders.TryAddWithoutValidation(key, values);
		}
		requestMessage.RequestUri = wr.ResponseUri;
		return httpResponseMessage;
	}

	private static bool MethodHasBody(HttpMethod method)
	{
		switch (method.Method)
		{
		case "HEAD":
		case "GET":
		case "MKCOL":
		case "CONNECT":
		case "TRACE":
			return false;
		default:
			return true;
		}
	}

	protected internal override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
	{
		if (disposed)
		{
			throw new ObjectDisposedException(GetType().ToString());
		}
		Volatile.Write(ref sentRequest, value: true);
		HttpWebRequest wrequest = CreateWebRequest(request);
		HttpWebResponse wresponse = null;
		try
		{
			using (cancellationToken.Register(delegate(object l)
			{
				((HttpWebRequest)l).Abort();
			}, wrequest))
			{
				HttpContent content = request.Content;
				if (content != null)
				{
					WebHeaderCollection headers = wrequest.Headers;
					foreach (KeyValuePair<string, IEnumerable<string>> header in content.Headers)
					{
						foreach (string item in header.Value)
						{
							headers.AddInternal(header.Key, item);
						}
					}
					if (request.Headers.TransferEncodingChunked == true)
					{
						wrequest.SendChunked = true;
					}
					else
					{
						long? contentLength = content.Headers.ContentLength;
						if (contentLength.HasValue)
						{
							wrequest.ContentLength = contentLength.Value;
						}
						else
						{
							if (MaxRequestContentBufferSize == 0L)
							{
								throw new InvalidOperationException("The content length of the request content can't be determined. Either set TransferEncodingChunked to true, load content into buffer, or set MaxRequestContentBufferSize.");
							}
							await content.LoadIntoBufferAsync(MaxRequestContentBufferSize).ConfigureAwait(continueOnCapturedContext: false);
							wrequest.ContentLength = content.Headers.ContentLength.Value;
						}
					}
					wrequest.ResendContentFactory = content.CopyTo;
					using Stream stream = await wrequest.GetRequestStreamAsync().ConfigureAwait(continueOnCapturedContext: false);
					await request.Content.CopyToAsync(stream).ConfigureAwait(continueOnCapturedContext: false);
				}
				else if (MethodHasBody(request.Method))
				{
					wrequest.ContentLength = 0L;
				}
				wresponse = (HttpWebResponse)(await wrequest.GetResponseAsync().ConfigureAwait(continueOnCapturedContext: false));
			}
		}
		catch (WebException ex)
		{
			if (ex.Status != WebExceptionStatus.RequestCanceled)
			{
				throw new HttpRequestException("An error occurred while sending the request", ex);
			}
		}
		catch (IOException inner)
		{
			throw new HttpRequestException("An error occurred while sending the request", inner);
		}
		if (cancellationToken.IsCancellationRequested)
		{
			TaskCompletionSource<HttpResponseMessage> taskCompletionSource = new TaskCompletionSource<HttpResponseMessage>();
			taskCompletionSource.SetCanceled();
			return await taskCompletionSource.Task;
		}
		return CreateResponseMessage(wresponse, request, cancellationToken);
	}
}
