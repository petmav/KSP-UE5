using System.Linq.Expressions;

namespace System.Runtime.CompilerServices;

public abstract class DebugInfoGenerator
{
	public static DebugInfoGenerator CreatePdbGenerator()
	{
		throw new PlatformNotSupportedException();
	}

	public abstract void MarkSequencePoint(LambdaExpression method, int ilOffset, DebugInfoExpression sequencePoint);
}
