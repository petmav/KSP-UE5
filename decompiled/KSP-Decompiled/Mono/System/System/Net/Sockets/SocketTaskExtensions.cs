using System.Collections.Generic;
using System.Threading.Tasks;

namespace System.Net.Sockets;

public static class SocketTaskExtensions
{
	public static Task<Socket> AcceptAsync(this Socket socket)
	{
		return Task<Socket>.Factory.FromAsync((AsyncCallback callback, object state) => ((Socket)state).BeginAccept(callback, state), (IAsyncResult asyncResult) => ((Socket)asyncResult.AsyncState).EndAccept(asyncResult), socket);
	}

	public static Task<Socket> AcceptAsync(this Socket socket, Socket acceptSocket)
	{
		return Task<Socket>.Factory.FromAsync((Socket socketForAccept, int receiveSize, AsyncCallback callback, object state) => ((Socket)state).BeginAccept(socketForAccept, receiveSize, callback, state), (IAsyncResult asyncResult) => ((Socket)asyncResult.AsyncState).EndAccept(asyncResult), acceptSocket, 0, socket);
	}

	public static Task ConnectAsync(this Socket socket, EndPoint remoteEP)
	{
		return Task.Factory.FromAsync((EndPoint targetEndPoint, AsyncCallback callback, object state) => ((Socket)state).BeginConnect(targetEndPoint, callback, state), delegate(IAsyncResult asyncResult)
		{
			((Socket)asyncResult.AsyncState).EndConnect(asyncResult);
		}, remoteEP, socket);
	}

	public static Task ConnectAsync(this Socket socket, IPAddress address, int port)
	{
		return Task.Factory.FromAsync((IPAddress targetAddress, int targetPort, AsyncCallback callback, object state) => ((Socket)state).BeginConnect(targetAddress, targetPort, callback, state), delegate(IAsyncResult asyncResult)
		{
			((Socket)asyncResult.AsyncState).EndConnect(asyncResult);
		}, address, port, socket);
	}

	public static Task ConnectAsync(this Socket socket, IPAddress[] addresses, int port)
	{
		return Task.Factory.FromAsync((IPAddress[] targetAddresses, int targetPort, AsyncCallback callback, object state) => ((Socket)state).BeginConnect(targetAddresses, targetPort, callback, state), delegate(IAsyncResult asyncResult)
		{
			((Socket)asyncResult.AsyncState).EndConnect(asyncResult);
		}, addresses, port, socket);
	}

	public static Task ConnectAsync(this Socket socket, string host, int port)
	{
		return Task.Factory.FromAsync((string targetHost, int targetPort, AsyncCallback callback, object state) => ((Socket)state).BeginConnect(targetHost, targetPort, callback, state), delegate(IAsyncResult asyncResult)
		{
			((Socket)asyncResult.AsyncState).EndConnect(asyncResult);
		}, host, port, socket);
	}

	public static Task<int> ReceiveAsync(this Socket socket, ArraySegment<byte> buffer, SocketFlags socketFlags)
	{
		return Task<int>.Factory.FromAsync((ArraySegment<byte> targetBuffer, SocketFlags flags, AsyncCallback callback, object state) => ((Socket)state).BeginReceive(targetBuffer.Array, targetBuffer.Offset, targetBuffer.Count, flags, callback, state), (IAsyncResult asyncResult) => ((Socket)asyncResult.AsyncState).EndReceive(asyncResult), buffer, socketFlags, socket);
	}

	public static Task<int> ReceiveAsync(this Socket socket, IList<ArraySegment<byte>> buffers, SocketFlags socketFlags)
	{
		return Task<int>.Factory.FromAsync((IList<ArraySegment<byte>> targetBuffers, SocketFlags flags, AsyncCallback callback, object state) => ((Socket)state).BeginReceive(targetBuffers, flags, callback, state), (IAsyncResult asyncResult) => ((Socket)asyncResult.AsyncState).EndReceive(asyncResult), buffers, socketFlags, socket);
	}

	public static Task<SocketReceiveFromResult> ReceiveFromAsync(this Socket socket, ArraySegment<byte> buffer, SocketFlags socketFlags, EndPoint remoteEndPoint)
	{
		object[] state2 = new object[2] { socket, remoteEndPoint };
		return Task<SocketReceiveFromResult>.Factory.FromAsync(delegate(ArraySegment<byte> targetBuffer, SocketFlags flags, AsyncCallback callback, object state)
		{
			object[] array = (object[])state;
			Socket socket3 = (Socket)array[0];
			EndPoint remoteEP = (EndPoint)array[1];
			IAsyncResult result2 = socket3.BeginReceiveFrom(targetBuffer.Array, targetBuffer.Offset, targetBuffer.Count, flags, ref remoteEP, callback, state);
			array[1] = remoteEP;
			return result2;
		}, delegate(IAsyncResult asyncResult)
		{
			object[] obj = (object[])asyncResult.AsyncState;
			Socket socket2 = (Socket)obj[0];
			EndPoint endPoint = (EndPoint)obj[1];
			int receivedBytes = socket2.EndReceiveFrom(asyncResult, ref endPoint);
			SocketReceiveFromResult result = default(SocketReceiveFromResult);
			result.ReceivedBytes = receivedBytes;
			result.RemoteEndPoint = endPoint;
			return result;
		}, buffer, socketFlags, state2);
	}

	public static Task<SocketReceiveMessageFromResult> ReceiveMessageFromAsync(this Socket socket, ArraySegment<byte> buffer, SocketFlags socketFlags, EndPoint remoteEndPoint)
	{
		object[] state2 = new object[3] { socket, socketFlags, remoteEndPoint };
		return Task<SocketReceiveMessageFromResult>.Factory.FromAsync(delegate(ArraySegment<byte> targetBuffer, AsyncCallback callback, object state)
		{
			object[] array = (object[])state;
			Socket socket3 = (Socket)array[0];
			SocketFlags socketFlags3 = (SocketFlags)array[1];
			EndPoint remoteEP = (EndPoint)array[2];
			IAsyncResult result2 = socket3.BeginReceiveMessageFrom(targetBuffer.Array, targetBuffer.Offset, targetBuffer.Count, socketFlags3, ref remoteEP, callback, state);
			array[2] = remoteEP;
			return result2;
		}, delegate(IAsyncResult asyncResult)
		{
			object[] obj = (object[])asyncResult.AsyncState;
			Socket socket2 = (Socket)obj[0];
			SocketFlags socketFlags2 = (SocketFlags)obj[1];
			EndPoint endPoint = (EndPoint)obj[2];
			IPPacketInformation ipPacketInformation;
			int receivedBytes = socket2.EndReceiveMessageFrom(asyncResult, ref socketFlags2, ref endPoint, out ipPacketInformation);
			SocketReceiveMessageFromResult result = default(SocketReceiveMessageFromResult);
			result.PacketInformation = ipPacketInformation;
			result.ReceivedBytes = receivedBytes;
			result.RemoteEndPoint = endPoint;
			result.SocketFlags = socketFlags2;
			return result;
		}, buffer, state2);
	}

	public static Task<int> SendAsync(this Socket socket, ArraySegment<byte> buffer, SocketFlags socketFlags)
	{
		return Task<int>.Factory.FromAsync((ArraySegment<byte> targetBuffer, SocketFlags flags, AsyncCallback callback, object state) => ((Socket)state).BeginSend(targetBuffer.Array, targetBuffer.Offset, targetBuffer.Count, flags, callback, state), (IAsyncResult asyncResult) => ((Socket)asyncResult.AsyncState).EndSend(asyncResult), buffer, socketFlags, socket);
	}

	public static Task<int> SendAsync(this Socket socket, IList<ArraySegment<byte>> buffers, SocketFlags socketFlags)
	{
		return Task<int>.Factory.FromAsync((IList<ArraySegment<byte>> targetBuffers, SocketFlags flags, AsyncCallback callback, object state) => ((Socket)state).BeginSend(targetBuffers, flags, callback, state), (IAsyncResult asyncResult) => ((Socket)asyncResult.AsyncState).EndSend(asyncResult), buffers, socketFlags, socket);
	}

	public static Task<int> SendToAsync(this Socket socket, ArraySegment<byte> buffer, SocketFlags socketFlags, EndPoint remoteEP)
	{
		return Task<int>.Factory.FromAsync((ArraySegment<byte> targetBuffer, SocketFlags flags, EndPoint endPoint, AsyncCallback callback, object state) => ((Socket)state).BeginSendTo(targetBuffer.Array, targetBuffer.Offset, targetBuffer.Count, flags, endPoint, callback, state), (IAsyncResult asyncResult) => ((Socket)asyncResult.AsyncState).EndSendTo(asyncResult), buffer, socketFlags, remoteEP, socket);
	}
}
