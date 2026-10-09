namespace System.Threading.Tasks;

public struct ParallelLoopResult
{
	internal bool m_completed;

	internal long? m_lowestBreakIteration;

	public bool IsCompleted => m_completed;

	public long? LowestBreakIteration => m_lowestBreakIteration;
}
