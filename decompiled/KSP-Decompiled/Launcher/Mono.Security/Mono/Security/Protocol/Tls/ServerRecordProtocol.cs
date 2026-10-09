using System;
using System.Globalization;
using System.IO;
using Mono.Security.Protocol.Tls.Handshake;
using Mono.Security.Protocol.Tls.Handshake.Server;

namespace Mono.Security.Protocol.Tls;

internal class ServerRecordProtocol : RecordProtocol
{
	private TlsClientCertificate cert;

	public ServerRecordProtocol(Stream innerStream, ServerContext context)
		: base(innerStream, context)
	{
	}

	public override HandshakeMessage GetMessage(HandshakeType type)
	{
		return createServerHandshakeMessage(type);
	}

	protected override void ProcessHandshakeMessage(TlsStream handMsg)
	{
		HandshakeType handshakeType = (HandshakeType)handMsg.ReadByte();
		HandshakeMessage handshakeMessage = null;
		int num = handMsg.ReadInt24();
		byte[] array = new byte[num];
		handMsg.Read(array, 0, num);
		handshakeMessage = createClientHandshakeMessage(handshakeType, array);
		handshakeMessage.Process();
		base.Context.LastHandshakeMsg = handshakeType;
		if (handshakeMessage != null)
		{
			handshakeMessage.Update();
			base.Context.HandshakeMessages.WriteByte((byte)handshakeType);
			base.Context.HandshakeMessages.WriteInt24(num);
			base.Context.HandshakeMessages.Write(array, 0, array.Length);
		}
	}

	private HandshakeMessage createClientHandshakeMessage(HandshakeType type, byte[] buffer)
	{
		HandshakeType lastHandshakeMsg = context.LastHandshakeMsg;
		switch (type)
		{
		case HandshakeType.ClientHello:
			return new TlsClientHello(context, buffer);
		case HandshakeType.Certificate:
			if (lastHandshakeMsg == HandshakeType.ClientHello)
			{
				cert = new TlsClientCertificate(context, buffer);
				return cert;
			}
			break;
		case HandshakeType.ClientKeyExchange:
			if (lastHandshakeMsg == HandshakeType.ClientHello || lastHandshakeMsg == HandshakeType.Certificate)
			{
				return new TlsClientKeyExchange(context, buffer);
			}
			break;
		case HandshakeType.CertificateVerify:
			if (lastHandshakeMsg == HandshakeType.ClientKeyExchange && cert != null)
			{
				return new TlsClientCertificateVerify(context, buffer);
			}
			break;
		case HandshakeType.Finished:
			if (((cert != null && cert.HasCertificate) ? (lastHandshakeMsg == HandshakeType.CertificateVerify) : (lastHandshakeMsg == HandshakeType.ClientKeyExchange)) && context.ChangeCipherSpecDone)
			{
				context.ChangeCipherSpecDone = false;
				return new TlsClientFinished(context, buffer);
			}
			break;
		default:
			throw new TlsException(AlertDescription.UnexpectedMessage, string.Format(CultureInfo.CurrentUICulture, "Unknown server handshake message received ({0})", type.ToString()));
		}
		throw new TlsException(AlertDescription.HandshakeFailiure, $"Protocol error, unexpected protocol transition from {lastHandshakeMsg} to {type}");
	}

	private HandshakeMessage createServerHandshakeMessage(HandshakeType type)
	{
		switch (type)
		{
		case HandshakeType.HelloRequest:
			SendRecord(HandshakeType.ClientHello);
			return null;
		case HandshakeType.ServerHello:
			return new TlsServerHello(context);
		case HandshakeType.Certificate:
			return new TlsServerCertificate(context);
		case HandshakeType.ServerKeyExchange:
			return new TlsServerKeyExchange(context);
		case HandshakeType.CertificateRequest:
			return new TlsServerCertificateRequest(context);
		case HandshakeType.ServerHelloDone:
			return new TlsServerHelloDone(context);
		case HandshakeType.Finished:
			return new TlsServerFinished(context);
		default:
			throw new InvalidOperationException("Unknown server handshake message type: " + type);
		}
	}
}
