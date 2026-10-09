using System.Collections;
using Unity;

namespace System.Security.Cryptography.Pkcs;

public sealed class RecipientInfoEnumerator : IEnumerator
{
	private IEnumerator enumerator;

	public RecipientInfo Current => (RecipientInfo)enumerator.Current;

	object IEnumerator.Current => enumerator.Current;

	internal RecipientInfoEnumerator(IEnumerable enumerable)
	{
		enumerator = enumerable.GetEnumerator();
	}

	public bool MoveNext()
	{
		return enumerator.MoveNext();
	}

	public void Reset()
	{
		enumerator.Reset();
	}

	internal RecipientInfoEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
