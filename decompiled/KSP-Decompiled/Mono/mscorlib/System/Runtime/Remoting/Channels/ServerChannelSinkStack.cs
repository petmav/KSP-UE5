using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.Remoting.Messaging;
using System.Security;

namespace System.Runtime.Remoting.Channels;

[ComVisible(true)]
public class ServerChannelSinkStack : IServerChannelSinkStack, IServerResponseChannelSinkStack
{
	private ChanelSinkStackEntry _sinkStack;

	[SecurityCritical]
	public Stream GetResponseStream(IMessage msg, ITransportHeaders headers)
	{
		if (_sinkStack == null)
		{
			throw new RemotingException("The sink stack is empty");
		}
		return ((IServerChannelSink)_sinkStack.Sink).GetResponseStream(this, _sinkStack.State, msg, headers);
	}

	[SecurityCritical]
	public object Pop(IServerChannelSink sink)
	{
		while (_sinkStack != null)
		{
			ChanelSinkStackEntry sinkStack = _sinkStack;
			_sinkStack = _sinkStack.Next;
			if (sinkStack.Sink == sink)
			{
				return sinkStack.State;
			}
		}
		throw new RemotingException("The current sink stack is empty, or the specified sink was never pushed onto the current stack");
	}

	[SecurityCritical]
	public void Push(IServerChannelSink sink, object state)
	{
		_sinkStack = new ChanelSinkStackEntry(sink, state, _sinkStack);
	}

	[MonoTODO]
	[SecurityCritical]
	public void ServerCallback(IAsyncResult ar)
	{
		throw new NotImplementedException();
	}

	[SecurityCritical]
	[MonoTODO]
	public void Store(IServerChannelSink sink, object state)
	{
		throw new NotImplementedException();
	}

	[SecurityCritical]
	[MonoTODO]
	public void StoreAndDispatch(IServerChannelSink sink, object state)
	{
		throw new NotImplementedException();
	}

	[SecurityCritical]
	public void AsyncProcessResponse(IMessage msg, ITransportHeaders headers, Stream stream)
	{
		if (_sinkStack == null)
		{
			throw new RemotingException("The current sink stack is empty");
		}
		ChanelSinkStackEntry sinkStack = _sinkStack;
		_sinkStack = _sinkStack.Next;
		((IServerChannelSink)sinkStack.Sink).AsyncProcessResponse(this, sinkStack.State, msg, headers, stream);
	}
}
