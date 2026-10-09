using System.ComponentModel;
using System.Diagnostics;

namespace System.Runtime.CompilerServices;

[DebuggerStepThrough]
[EditorBrowsable(EditorBrowsableState.Never)]
public sealed class Closure
{
	public readonly object[] Constants;

	public readonly object[] Locals;

	public Closure(object[] constants, object[] locals)
	{
		Constants = constants;
		Locals = locals;
	}
}
