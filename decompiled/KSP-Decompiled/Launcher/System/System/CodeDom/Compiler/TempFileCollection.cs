using System.Collections;
using System.Runtime.CompilerServices;
using System.Security.Permissions;
using Unity;

namespace System.CodeDom.Compiler;

[Serializable]
[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class TempFileCollection : ICollection, IEnumerable, IDisposable
{
	public string BasePath
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public int Count
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public bool KeepFiles
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string TempDir
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public TempFileCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public TempFileCollection(string tempDir)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public TempFileCollection(string tempDir, bool keepFiles)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	int ICollection.get_Count()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	[SpecialName]
	bool ICollection.get_IsSynchronized()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SpecialName]
	object ICollection.get_SyncRoot()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public string AddExtension(string fileExtension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public string AddExtension(string fileExtension, bool keepFile)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void AddFile(string fileName, bool keepFile)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CopyTo(string[] fileNames, int start)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Delete()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void Dispose(bool disposing)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public IEnumerator GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	void ICollection.CopyTo(Array array, int start)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	void IDisposable.Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
