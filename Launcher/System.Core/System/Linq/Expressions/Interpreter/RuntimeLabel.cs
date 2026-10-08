using System.Globalization;

namespace System.Linq.Expressions.Interpreter;

internal struct RuntimeLabel
{
	public readonly int Index;

	public readonly int StackDepth;

	public readonly int ContinuationStackDepth;

	public RuntimeLabel(int index, int continuationStackDepth, int stackDepth)
	{
		Index = index;
		ContinuationStackDepth = continuationStackDepth;
		StackDepth = stackDepth;
	}

	public override string ToString()
	{
		return string.Format(CultureInfo.InvariantCulture, "->{0} C({1}) S({2})", Index, ContinuationStackDepth, StackDepth);
	}
}
