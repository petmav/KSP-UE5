using System.Runtime.InteropServices;

namespace System.Reflection;

[ComVisible(true)]
[Flags]
public enum ExceptionHandlingClauseOptions
{
	Clause = 0,
	Filter = 1,
	Finally = 2,
	Fault = 4
}
