using System.Collections;
using System.Runtime.InteropServices;
using Unity;

namespace System.Security.Policy;

[ComVisible(true)]
public sealed class ApplicationTrustEnumerator : IEnumerator
{
	private IEnumerator e;

	public ApplicationTrust Current => (ApplicationTrust)e.Current;

	object IEnumerator.Current => e.Current;

	internal ApplicationTrustEnumerator(ApplicationTrustCollection collection)
	{
		e = collection.GetEnumerator();
	}

	[SecuritySafeCritical]
	public bool MoveNext()
	{
		return e.MoveNext();
	}

	public void Reset()
	{
		e.Reset();
	}

	internal ApplicationTrustEnumerator()
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
