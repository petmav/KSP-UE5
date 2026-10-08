using System.Collections;
using Unity;

namespace System.Security.Cryptography.Pkcs;

public sealed class SignerInfoEnumerator : IEnumerator
{
	private IEnumerator enumerator;

	public SignerInfo Current => (SignerInfo)enumerator.Current;

	object IEnumerator.Current => enumerator.Current;

	internal SignerInfoEnumerator(IEnumerable enumerable)
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

	internal SignerInfoEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
