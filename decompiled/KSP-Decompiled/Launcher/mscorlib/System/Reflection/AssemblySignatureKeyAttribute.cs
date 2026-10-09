namespace System.Reflection;

[AttributeUsage(AttributeTargets.Assembly, Inherited = false, AllowMultiple = false)]
public sealed class AssemblySignatureKeyAttribute : Attribute
{
	private string _publicKey;

	private string _countersignature;

	public string PublicKey => _publicKey;

	public string Countersignature => _countersignature;

	public AssemblySignatureKeyAttribute(string publicKey, string countersignature)
	{
		_publicKey = publicKey;
		_countersignature = countersignature;
	}
}
