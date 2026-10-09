using System.Diagnostics;

namespace System.Linq.Expressions.Interpreter;

internal static class Assert
{
	[Conditional("DEBUG")]
	public static void NotNull(object var)
	{
	}
}
