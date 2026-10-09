namespace System.IO.Pipes;

internal interface INamedPipeClient : IPipe
{
	int NumberOfServerInstances { get; }

	bool IsAsync { get; }

	void Connect();

	void Connect(int timeout);
}
