using Unity;

namespace System.Security.Cryptography.Pkcs;

public sealed class SubjectIdentifierOrKey
{
	private SubjectIdentifierOrKeyType _type;

	private object _value;

	public SubjectIdentifierOrKeyType Type => _type;

	public object Value => _value;

	internal SubjectIdentifierOrKey(SubjectIdentifierOrKeyType type, object value)
	{
		_type = type;
		_value = value;
	}

	internal SubjectIdentifierOrKey()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
