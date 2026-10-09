using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Security.Authentication;
using Mono.Security.Interface;

namespace Mono.Net.Security;

internal class MonoTlsStream
{
	private readonly MonoTlsProvider provider;

	private readonly NetworkStream networkStream;

	private readonly HttpWebRequest request;

	private readonly MonoTlsSettings settings;

	private IMonoSslStream sslStream;

	private WebExceptionStatus status;

	internal HttpWebRequest Request => request;

	internal IMonoSslStream SslStream => sslStream;

	internal WebExceptionStatus ExceptionStatus => status;

	internal bool CertificateValidationFailed { get; set; }

	public MonoTlsStream(HttpWebRequest request, NetworkStream networkStream)
	{
		this.request = request;
		this.networkStream = networkStream;
		settings = request.TlsSettings;
		provider = request.TlsProvider ?? MonoTlsProviderFactory.GetProviderInternal();
		status = WebExceptionStatus.SecureChannelFailure;
		ChainValidationHelper.Create(provider, ref settings, this);
	}

	internal Stream CreateStream(byte[] buffer)
	{
		sslStream = provider.CreateSslStream(networkStream, leaveInnerStreamOpen: false, settings);
		try
		{
			string text = request.Host;
			if (!string.IsNullOrEmpty(text))
			{
				int num = text.IndexOf(':');
				if (num > 0)
				{
					text = text.Substring(0, num);
				}
			}
			sslStream.AuthenticateAsClient(text, request.ClientCertificates, (SslProtocols)ServicePointManager.SecurityProtocol, ServicePointManager.CheckCertificateRevocationList);
			status = WebExceptionStatus.Success;
		}
		catch
		{
			status = WebExceptionStatus.SecureChannelFailure;
			throw;
		}
		finally
		{
			if (CertificateValidationFailed)
			{
				status = WebExceptionStatus.TrustFailure;
			}
			if (status == WebExceptionStatus.Success)
			{
				request.ServicePoint.UpdateClientCertificate(sslStream.InternalLocalCertificate);
			}
			else
			{
				request.ServicePoint.UpdateClientCertificate(null);
				sslStream = null;
			}
		}
		try
		{
			if (buffer != null)
			{
				sslStream.Write(buffer, 0, buffer.Length);
			}
		}
		catch
		{
			status = WebExceptionStatus.SendFailure;
			sslStream = null;
			throw;
		}
		return sslStream.AuthenticatedStream;
	}
}
