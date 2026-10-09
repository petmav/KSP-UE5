using System.Collections.Generic;
using Unity;

namespace System.Runtime.Serialization;

public sealed class SafeSerializationEventArgs : EventArgs
{
	private StreamingContext m_streamingContext;

	private List<object> m_serializedStates;

	internal IList<object> SerializedStates => m_serializedStates;

	public StreamingContext StreamingContext => m_streamingContext;

	internal SafeSerializationEventArgs(StreamingContext streamingContext)
	{
		m_serializedStates = new List<object>();
		base._002Ector();
		m_streamingContext = streamingContext;
	}

	public void AddSerializedState(ISafeSerializationData serializedState)
	{
		if (serializedState == null)
		{
			throw new ArgumentNullException("serializedState");
		}
		if (!serializedState.GetType().IsSerializable)
		{
			throw new ArgumentException(Environment.GetResourceString("Type '{0}' in Assembly '{1}' is not marked as serializable.", serializedState.GetType(), serializedState.GetType().Assembly.FullName));
		}
		m_serializedStates.Add(serializedState);
	}

	internal SafeSerializationEventArgs()
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
