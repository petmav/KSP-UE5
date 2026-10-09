using System.Runtime.CompilerServices;

namespace System.Collections.Generic;

[TypeDependency("System.SZArrayHelper")]
public interface IEnumerable<out T> : IEnumerable
{
	new IEnumerator<T> GetEnumerator();
}
