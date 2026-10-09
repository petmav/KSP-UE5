namespace System.Threading.Tasks;

internal interface IDecoupledTask
{
	bool IsCompleted { get; }
}
