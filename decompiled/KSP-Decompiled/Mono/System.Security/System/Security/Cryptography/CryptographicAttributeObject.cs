namespace System.Security.Cryptography;

public sealed class CryptographicAttributeObject
{
	private Oid _oid;

	private AsnEncodedDataCollection _list;

	public Oid Oid => _oid;

	public AsnEncodedDataCollection Values => _list;

	public CryptographicAttributeObject(Oid oid)
	{
		if (oid == null)
		{
			throw new ArgumentNullException("oid");
		}
		_oid = new Oid(oid);
		_list = new AsnEncodedDataCollection();
	}

	public CryptographicAttributeObject(Oid oid, AsnEncodedDataCollection values)
	{
		if (oid == null)
		{
			throw new ArgumentNullException("oid");
		}
		_oid = new Oid(oid);
		if (values == null)
		{
			_list = new AsnEncodedDataCollection();
		}
		else
		{
			_list = values;
		}
	}
}
