using System;
using System.Globalization;
using System.IO;
using Mono.Security.Protocol.Tls.Handshake;
using Mono.Security.Protocol.Tls.Handshake.Client;

namespace Mono.Security.Protocol.Tls;

internal class ClientRecordProtocol : RecordProtocol
{
	public ClientRecordProtocol(Stream innerStream, ClientContext context)
		: base(innerStream, context)
	{
	}

	public override HandshakeMessage GetMessage(HandshakeType type)
	{
		return createClientHandshakeMessage(type);
	}

	protected override void ProcessHandshakeMessage(TlsStream handMsg)
	{
		HandshakeType handshakeType = (HandshakeType)handMsg.ReadByte();
		HandshakeMessage handshakeMessage = null;
		int num = handMsg.ReadInt24();
		byte[] array = null;
		if (num > 0)
		{
			array = new byte[num];
			handMsg.Read(array, 0, num);
		}
		handshakeMessage = createServerHandshakeMessage(handshakeType, array);
		handshakeMessage?.Process();
		base.Context.LastHandshakeMsg = handshakeType;
		if (handshakeMessage != null)
		{
			handshakeMessage.Update();
			base.Context.HandshakeMessages.WriteByte((byte)handshakeType);
			base.Context.HandshakeMessages.WriteInt24(num);
			if (num > 0)
			{
				base.Context.HandshakeMessages.Write(array, 0, array.Length);
			}
		}
	}

	private HandshakeMessage createClientHandshakeMessage(HandshakeType type)
	{
		return type switch
		{
			HandshakeType.ClientHello => new TlsClientHello(context), 
			HandshakeType.Certificate => new TlsClientCertificate(context), 
			HandshakeType.ClientKeyExchange => new TlsClientKeyExchange(context), 
			HandshakeType.CertificateVerify => new TlsClientCertificateVerify(context), 
			HandshakeType.Finished => new TlsClientFinished(context), 
			_ => throw new InvalidOperationException("Unknown client handshake message type: " + type), 
		};
	}

	private HandshakeMessage createServerHandshakeMessage(HandshakeType type, byte[] buffer)
	{
		ClientContext clientContext = (ClientContext)context;
		HandshakeType lastHandshakeMsg = clientContext.LastHandshakeMsg;
		switch (type)
		{
		case HandshakeType.HelloRequest:
			if (clientContext.HandshakeState != HandshakeState.Started)
			{
				clientContext.HandshakeState = HandshakeState.None;
			}
			else
			{
				SendAlert(AlertLevel.Warning, AlertDescription.NoRenegotiation);
			}
			return null;
		case HandshakeType.ServerHello:
			if (lastHandshakeMsg == HandshakeType.HelloRequest)
			{
				return new TlsServerHello(context, buffer);
			}
			break;
		case HandshakeType.Certificate:
			if (lastHandshakeMsg == HandshakeType.ServerHello)
			{
				return new TlsServerCertificate(context, buffer);
			}
			break;
		case HandshakeType.CertificateRequest:
			if (lastHandshakeMsg == HandshakeType.ServerKeyExchange || lastHandshakeMsg == HandshakeType.Certificate)
			{
				return new TlsServerCertificateRequest(context, buffer);
			}
			break;
		case HandshakeType.ServerHelloDone:
			if (lastHandshakeMsg == HandshakeType.CertificateRequest || lastHandshakeMsg == HandshakeType.Certificate || lastHandshakeMsg == HandshakeType.ServerHello)
			{
				return new TlsServerHelloDone(context, buffer);
			}
			break;
		case HandshakeType.Finished:
			if ((clientContext.AbbreviatedHandshake ? (lastHandshakeMsg == HandshakeType.ServerHello) : (lastHandshakeMsg == HandshakeType.ServerHelloDone)) && clientContext.ChangeCipherSpecDone)
			{
				clientContext.ChangeCipherSpecDone = false;
				return new TlsServerFinished(context, buffer);
			}
			break;
		default:
			throw new TlsException(AlertDescription.UnexpectedMessage, string.Format(CultureInfo.CurrentUICulture, "Unknown server handshake message received ({0})", type.ToString()));
		}
		throw new TlsException(AlertDescription.HandshakeFailiure, $"Protocol error, unexpected protocol transition from {lastHandshakeMsg} to {type}");
	}
}
