using System.Collections;
using Unity;

namespace System.Security.Cryptography;

public sealed class CryptographicAttributeObjectEnumerator : IEnumerator
{
	private IEnumerator enumerator;

	public CryptographicAttributeObject Current => (CryptographicAttributeObject)enumerator.Current;

	object IEnumerator.Current => enumerator.Current;

	internal CryptographicAttributeObjectEnumerator(IEnumerable enumerable)
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

	internal CryptographicAttributeObjectEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
