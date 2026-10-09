namespace System.Net;

internal class InternalException : Exception
{
	internal InternalException()
	{
		System.Net.NetEventSource.Fail(this, "InternalException thrown.", ".ctor");
	}
}
