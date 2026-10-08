using Unity;

namespace System.IO.Ports;

public class SerialDataReceivedEventArgs : EventArgs
{
	public SerialData EventType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SerialData);
		}
	}

	internal SerialDataReceivedEventArgs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
