using System.Collections;
using System.IO;

namespace System.CodeDom.Compiler;

[Serializable]
public class TempFileCollection : ICollection, IEnumerable, IDisposable
{
	private string _basePath;

	private readonly string _tempDir;

	private readonly Hashtable _files;

	public int Count => _files.Count;

	int ICollection.Count => _files.Count;

	object ICollection.SyncRoot => null;

	bool ICollection.IsSynchronized => false;

	public string TempDir => _tempDir ?? string.Empty;

	public string BasePath
	{
		get
		{
			EnsureTempNameCreated();
			return _basePath;
		}
	}

	public bool KeepFiles { get; set; }

	public TempFileCollection()
		: this(null, keepFiles: false)
	{
	}

	public TempFileCollection(string tempDir)
		: this(tempDir, keepFiles: false)
	{
	}

	public TempFileCollection(string tempDir, bool keepFiles)
	{
		KeepFiles = keepFiles;
		_tempDir = tempDir;
		_files = new Hashtable(StringComparer.OrdinalIgnoreCase);
	}

	void IDisposable.Dispose()
	{
		Dispose(disposing: true);
		GC.SuppressFinalize(this);
	}

	protected virtual void Dispose(bool disposing)
	{
		SafeDelete();
	}

	~TempFileCollection()
	{
		Dispose(disposing: false);
	}

	public string AddExtension(string fileExtension)
	{
		return AddExtension(fileExtension, KeepFiles);
	}

	public string AddExtension(string fileExtension, bool keepFile)
	{
		if (string.IsNullOrEmpty(fileExtension))
		{
			throw new ArgumentException(global::SR.Format("Argument {0} cannot be null or zero-length.", "fileExtension"), "fileExtension");
		}
		string text = BasePath + "." + fileExtension;
		AddFile(text, keepFile);
		return text;
	}

	public void AddFile(string fileName, bool keepFile)
	{
		if (string.IsNullOrEmpty(fileName))
		{
			throw new ArgumentException(global::SR.Format("Argument {0} cannot be null or zero-length.", "fileName"), "fileName");
		}
		if (_files[fileName] != null)
		{
			throw new ArgumentException(global::SR.Format("The file name '{0}' was already in the collection.", fileName), "fileName");
		}
		_files.Add(fileName, keepFile);
	}

	public IEnumerator GetEnumerator()
	{
		return _files.Keys.GetEnumerator();
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		return _files.Keys.GetEnumerator();
	}

	void ICollection.CopyTo(Array array, int start)
	{
		_files.Keys.CopyTo(array, start);
	}

	public void CopyTo(string[] fileNames, int start)
	{
		_files.Keys.CopyTo(fileNames, start);
	}

	private void EnsureTempNameCreated()
	{
		if (_basePath != null)
		{
			return;
		}
		string text = null;
		bool flag = false;
		int num = 5000;
		do
		{
			_basePath = Path.Combine(string.IsNullOrEmpty(TempDir) ? Path.GetTempPath() : TempDir, Path.GetFileNameWithoutExtension(Path.GetRandomFileName()));
			text = _basePath + ".tmp";
			try
			{
				new FileStream(text, FileMode.CreateNew, FileAccess.Write).Dispose();
				flag = true;
			}
			catch (IOException ex)
			{
				num--;
				if (num == 0 || ex is DirectoryNotFoundException)
				{
					throw;
				}
				flag = false;
			}
		}
		while (!flag);
		_files.Add(text, KeepFiles);
	}

	private bool KeepFile(string fileName)
	{
		object obj = _files[fileName];
		if (obj == null)
		{
			return false;
		}
		return (bool)obj;
	}

	public void Delete()
	{
		SafeDelete();
	}

	internal void Delete(string fileName)
	{
		try
		{
			File.Delete(fileName);
		}
		catch
		{
		}
	}

	internal void SafeDelete()
	{
		if (_files == null || _files.Count <= 0)
		{
			return;
		}
		string[] array = new string[_files.Count];
		_files.Keys.CopyTo(array, 0);
		string[] array2 = array;
		foreach (string text in array2)
		{
			if (!KeepFile(text))
			{
				Delete(text);
				_files.Remove(text);
			}
		}
	}
}
