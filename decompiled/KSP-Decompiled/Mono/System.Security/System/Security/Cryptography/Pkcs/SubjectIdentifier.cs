using Unity;

namespace System.Security.Cryptography.Pkcs;

public sealed class SubjectIdentifier
{
	private SubjectIdentifierType _type;

	private object _value;

	public SubjectIdentifierType Type => _type;

	public object Value => _value;

	internal SubjectIdentifier(SubjectIdentifierType type, object value)
	{
		_type = type;
		_value = value;
	}

	internal SubjectIdentifier()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
