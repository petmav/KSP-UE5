using Unity;

namespace System.IO.Ports;

public class SerialErrorReceivedEventArgs : EventArgs
{
	public SerialError EventType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SerialError);
		}
	}

	internal SerialErrorReceivedEventArgs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
