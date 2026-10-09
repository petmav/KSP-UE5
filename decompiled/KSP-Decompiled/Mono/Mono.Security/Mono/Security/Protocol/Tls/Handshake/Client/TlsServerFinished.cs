using Mono.Security.Cryptography;

namespace Mono.Security.Protocol.Tls.Handshake.Client;

internal class TlsServerFinished : HandshakeMessage
{
	private static byte[] Ssl3Marker = new byte[4] { 83, 82, 86, 82 };

	public TlsServerFinished(Context context, byte[] buffer)
		: base(context, HandshakeType.Finished, buffer)
	{
	}

	public override void Update()
	{
		base.Update();
		base.Context.HandshakeState = HandshakeState.Finished;
	}

	protected override void ProcessAsSsl3()
	{
		SslHandshakeHash sslHandshakeHash = new SslHandshakeHash(base.Context.MasterSecret);
		byte[] array = base.Context.HandshakeMessages.ToArray();
		sslHandshakeHash.TransformBlock(array, 0, array.Length, array, 0);
		sslHandshakeHash.TransformBlock(Ssl3Marker, 0, Ssl3Marker.Length, Ssl3Marker, 0);
		sslHandshakeHash.TransformFinalBlock(CipherSuite.EmptyArray, 0, 0);
		if (!HandshakeMessage.Compare(buffer2: ReadBytes((int)Length), buffer1: sslHandshakeHash.Hash))
		{
			throw new TlsException(AlertDescription.InsuficientSecurity, "Invalid ServerFinished message received.");
		}
	}

	protected override void ProcessAsTls1()
	{
		byte[] buffer = ReadBytes((int)Length);
		MD5SHA1 mD5SHA = new MD5SHA1();
		byte[] array = base.Context.HandshakeMessages.ToArray();
		byte[] data = mD5SHA.ComputeHash(array, 0, array.Length);
		if (!HandshakeMessage.Compare(base.Context.Current.Cipher.PRF(base.Context.MasterSecret, "server finished", data, 12), buffer))
		{
			throw new TlsException("Invalid ServerFinished message received.");
		}
	}
}
