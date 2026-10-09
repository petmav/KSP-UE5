using System.Collections;
using Unity;

namespace System.Security.Cryptography.Pkcs;

public sealed class CmsRecipientEnumerator : IEnumerator
{
	private IEnumerator enumerator;

	public CmsRecipient Current => (CmsRecipient)enumerator.Current;

	object IEnumerator.Current => enumerator.Current;

	internal CmsRecipientEnumerator(IEnumerable enumerable)
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

	internal CmsRecipientEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
