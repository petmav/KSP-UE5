using System.Security.Cryptography.X509Certificates;

namespace System.Security.Cryptography.Pkcs;

public sealed class CmsRecipient
{
	private SubjectIdentifierType _recipient;

	private X509Certificate2 _certificate;

	public X509Certificate2 Certificate => _certificate;

	public SubjectIdentifierType RecipientIdentifierType => _recipient;

	public CmsRecipient(X509Certificate2 certificate)
	{
		if (certificate == null)
		{
			throw new ArgumentNullException("certificate");
		}
		_recipient = SubjectIdentifierType.IssuerAndSerialNumber;
		_certificate = certificate;
	}

	public CmsRecipient(SubjectIdentifierType recipientIdentifierType, X509Certificate2 certificate)
	{
		if (certificate == null)
		{
			throw new ArgumentNullException("certificate");
		}
		if (recipientIdentifierType == SubjectIdentifierType.Unknown)
		{
			_recipient = SubjectIdentifierType.IssuerAndSerialNumber;
		}
		else
		{
			_recipient = recipientIdentifierType;
		}
		_certificate = certificate;
	}
}
