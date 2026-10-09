using System.Collections.Generic;
using Unity;

namespace System.Security.Cryptography;

public abstract class DataProtector
{
	protected string ApplicationName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected virtual bool PrependHashedPurposeToPlaintext
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	protected string PrimaryPurpose
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected IEnumerable<string> SpecificPurposes
	{
		get
		{
			//IL_0007: Expected O, but got I4
			Unity.ThrowStub.ThrowNotSupportedException();
			return (IEnumerable<string>)0;
		}
	}

	protected DataProtector(string applicationName, string primaryPurpose, string[] specificPurposes)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static DataProtector Create(string providerClass, string applicationName, string primaryPurpose, string[] specificPurposes)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual byte[] GetHashedPurpose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public abstract bool IsReprotectRequired(byte[] encryptedData);

	public byte[] Protect(byte[] userData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected abstract byte[] ProviderProtect(byte[] userData);

	protected abstract byte[] ProviderUnprotect(byte[] encryptedData);

	public byte[] Unprotect(byte[] encryptedData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
