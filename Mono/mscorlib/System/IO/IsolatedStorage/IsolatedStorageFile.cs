using System.Collections;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Serialization.Formatters.Binary;
using System.Security;
using System.Security.Cryptography;
using System.Security.Permissions;
using System.Security.Policy;
using System.Text;
using System.Threading;
using Mono.Security.Cryptography;
using Unity;

namespace System.IO.IsolatedStorage;

[ComVisible(true)]
[FileIOPermission(SecurityAction.Assert, Unrestricted = true)]
public sealed class IsolatedStorageFile : IsolatedStorage, IDisposable
{
	[Serializable]
	private struct Identities
	{
		public object Application;

		public object Assembly;

		public object Domain;

		public Identities(object application, object assembly, object domain)
		{
			Application = application;
			Assembly = assembly;
			Domain = domain;
		}
	}

	private bool _resolved;

	private ulong _maxSize;

	private Evidence _fullEvidences;

	private static readonly Mutex mutex = new Mutex();

	private bool closed;

	private bool disposed;

	private DirectoryInfo directory;

	[Obsolete]
	[CLSCompliant(false)]
	public override ulong CurrentSize => GetDirectorySize(directory);

	[Obsolete]
	[CLSCompliant(false)]
	public override ulong MaximumSize
	{
		get
		{
			if (!SecurityManager.SecurityEnabled)
			{
				return 9223372036854775807uL;
			}
			if (_resolved)
			{
				return _maxSize;
			}
			Evidence evidence = null;
			if (_fullEvidences != null)
			{
				evidence = _fullEvidences;
			}
			else
			{
				evidence = new Evidence();
				if (_assemblyIdentity != null)
				{
					evidence.AddHost(_assemblyIdentity);
				}
			}
			if (evidence.Count < 1)
			{
				throw new InvalidOperationException(Locale.GetText("Couldn't get the quota from the available evidences."));
			}
			PermissionSet denied = null;
			PermissionSet permissionSet = SecurityManager.ResolvePolicy(evidence, null, null, null, out denied);
			IsolatedStoragePermission permission = GetPermission(permissionSet);
			if (permission == null)
			{
				if (!permissionSet.IsUnrestricted())
				{
					throw new InvalidOperationException(Locale.GetText("No quota from the available evidences."));
				}
				_maxSize = 9223372036854775807uL;
			}
			else
			{
				_maxSize = (ulong)permission.UserQuota;
			}
			_resolved = true;
			return _maxSize;
		}
	}

	internal string Root => directory.FullName;

	[ComVisible(false)]
	public override long AvailableFreeSpace
	{
		get
		{
			CheckOpen();
			return long.MaxValue;
		}
	}

	[ComVisible(false)]
	public override long Quota
	{
		get
		{
			CheckOpen();
			return (long)MaximumSize;
		}
	}

	[ComVisible(false)]
	public override long UsedSize
	{
		get
		{
			CheckOpen();
			return (long)GetDirectorySize(directory);
		}
	}

	[ComVisible(false)]
	public static bool IsEnabled => true;

	internal bool IsClosed => closed;

	internal bool IsDisposed => disposed;

	public static IEnumerator GetEnumerator(IsolatedStorageScope scope)
	{
		Demand(scope);
		if (scope != IsolatedStorageScope.User && scope != (IsolatedStorageScope.User | IsolatedStorageScope.Roaming) && scope != IsolatedStorageScope.Machine)
		{
			throw new ArgumentException(Locale.GetText("Invalid scope, only User, User|Roaming and Machine are valid"));
		}
		return new IsolatedStorageFileEnumerator(scope, GetIsolatedStorageRoot(scope));
	}

	public static IsolatedStorageFile GetStore(IsolatedStorageScope scope, Evidence domainEvidence, Type domainEvidenceType, Evidence assemblyEvidence, Type assemblyEvidenceType)
	{
		Demand(scope);
		bool num = (scope & IsolatedStorageScope.Domain) != 0;
		if (num && domainEvidence == null)
		{
			throw new ArgumentNullException("domainEvidence");
		}
		bool flag = (scope & IsolatedStorageScope.Assembly) != 0;
		if (flag && assemblyEvidence == null)
		{
			throw new ArgumentNullException("assemblyEvidence");
		}
		IsolatedStorageFile isolatedStorageFile = new IsolatedStorageFile(scope);
		if (num)
		{
			if (domainEvidenceType == null)
			{
				isolatedStorageFile._domainIdentity = GetDomainIdentityFromEvidence(domainEvidence);
			}
			else
			{
				isolatedStorageFile._domainIdentity = GetTypeFromEvidence(domainEvidence, domainEvidenceType);
			}
			if (isolatedStorageFile._domainIdentity == null)
			{
				throw new IsolatedStorageException(Locale.GetText("Couldn't find domain identity."));
			}
		}
		if (flag)
		{
			if (assemblyEvidenceType == null)
			{
				isolatedStorageFile._assemblyIdentity = GetAssemblyIdentityFromEvidence(assemblyEvidence);
			}
			else
			{
				isolatedStorageFile._assemblyIdentity = GetTypeFromEvidence(assemblyEvidence, assemblyEvidenceType);
			}
			if (isolatedStorageFile._assemblyIdentity == null)
			{
				throw new IsolatedStorageException(Locale.GetText("Couldn't find assembly identity."));
			}
		}
		isolatedStorageFile.PostInit();
		return isolatedStorageFile;
	}

	public static IsolatedStorageFile GetStore(IsolatedStorageScope scope, object domainIdentity, object assemblyIdentity)
	{
		Demand(scope);
		if ((scope & IsolatedStorageScope.Domain) != 0 && domainIdentity == null)
		{
			throw new ArgumentNullException("domainIdentity");
		}
		bool num = (scope & IsolatedStorageScope.Assembly) != 0;
		if (num && assemblyIdentity == null)
		{
			throw new ArgumentNullException("assemblyIdentity");
		}
		IsolatedStorageFile isolatedStorageFile = new IsolatedStorageFile(scope);
		if (num)
		{
			isolatedStorageFile._fullEvidences = Assembly.GetCallingAssembly().UnprotectedGetEvidence();
		}
		isolatedStorageFile._domainIdentity = domainIdentity;
		isolatedStorageFile._assemblyIdentity = assemblyIdentity;
		isolatedStorageFile.PostInit();
		return isolatedStorageFile;
	}

	public static IsolatedStorageFile GetStore(IsolatedStorageScope scope, Type domainEvidenceType, Type assemblyEvidenceType)
	{
		Demand(scope);
		IsolatedStorageFile isolatedStorageFile = new IsolatedStorageFile(scope);
		if ((scope & IsolatedStorageScope.Domain) != 0)
		{
			if (domainEvidenceType == null)
			{
				domainEvidenceType = typeof(Url);
			}
			isolatedStorageFile._domainIdentity = GetTypeFromEvidence(AppDomain.CurrentDomain.Evidence, domainEvidenceType);
		}
		if ((scope & IsolatedStorageScope.Assembly) != 0)
		{
			Evidence e = (isolatedStorageFile._fullEvidences = Assembly.GetCallingAssembly().UnprotectedGetEvidence());
			if ((scope & IsolatedStorageScope.Domain) != 0)
			{
				if (assemblyEvidenceType == null)
				{
					assemblyEvidenceType = typeof(Url);
				}
				isolatedStorageFile._assemblyIdentity = GetTypeFromEvidence(e, assemblyEvidenceType);
			}
			else
			{
				isolatedStorageFile._assemblyIdentity = GetAssemblyIdentityFromEvidence(e);
			}
		}
		isolatedStorageFile.PostInit();
		return isolatedStorageFile;
	}

	public static IsolatedStorageFile GetStore(IsolatedStorageScope scope, object applicationIdentity)
	{
		Demand(scope);
		if (applicationIdentity == null)
		{
			throw new ArgumentNullException("applicationIdentity");
		}
		IsolatedStorageFile isolatedStorageFile = new IsolatedStorageFile(scope);
		isolatedStorageFile._applicationIdentity = applicationIdentity;
		isolatedStorageFile._fullEvidences = Assembly.GetCallingAssembly().UnprotectedGetEvidence();
		isolatedStorageFile.PostInit();
		return isolatedStorageFile;
	}

	public static IsolatedStorageFile GetStore(IsolatedStorageScope scope, Type applicationEvidenceType)
	{
		Demand(scope);
		IsolatedStorageFile isolatedStorageFile = new IsolatedStorageFile(scope);
		isolatedStorageFile.InitStore(scope, applicationEvidenceType);
		isolatedStorageFile._fullEvidences = Assembly.GetCallingAssembly().UnprotectedGetEvidence();
		isolatedStorageFile.PostInit();
		return isolatedStorageFile;
	}

	[IsolatedStorageFilePermission(SecurityAction.Demand, UsageAllowed = IsolatedStorageContainment.ApplicationIsolationByMachine)]
	public static IsolatedStorageFile GetMachineStoreForApplication()
	{
		IsolatedStorageScope scope = IsolatedStorageScope.Machine | IsolatedStorageScope.Application;
		IsolatedStorageFile isolatedStorageFile = new IsolatedStorageFile(scope);
		isolatedStorageFile.InitStore(scope, null);
		isolatedStorageFile._fullEvidences = Assembly.GetCallingAssembly().UnprotectedGetEvidence();
		isolatedStorageFile.PostInit();
		return isolatedStorageFile;
	}

	[IsolatedStorageFilePermission(SecurityAction.Demand, UsageAllowed = IsolatedStorageContainment.AssemblyIsolationByMachine)]
	public static IsolatedStorageFile GetMachineStoreForAssembly()
	{
		IsolatedStorageFile isolatedStorageFile = new IsolatedStorageFile(IsolatedStorageScope.Assembly | IsolatedStorageScope.Machine);
		isolatedStorageFile._assemblyIdentity = GetAssemblyIdentityFromEvidence(isolatedStorageFile._fullEvidences = Assembly.GetCallingAssembly().UnprotectedGetEvidence());
		isolatedStorageFile.PostInit();
		return isolatedStorageFile;
	}

	[IsolatedStorageFilePermission(SecurityAction.Demand, UsageAllowed = IsolatedStorageContainment.DomainIsolationByMachine)]
	public static IsolatedStorageFile GetMachineStoreForDomain()
	{
		IsolatedStorageFile isolatedStorageFile = new IsolatedStorageFile(IsolatedStorageScope.Domain | IsolatedStorageScope.Assembly | IsolatedStorageScope.Machine);
		isolatedStorageFile._domainIdentity = GetDomainIdentityFromEvidence(AppDomain.CurrentDomain.Evidence);
		isolatedStorageFile._assemblyIdentity = GetAssemblyIdentityFromEvidence(isolatedStorageFile._fullEvidences = Assembly.GetCallingAssembly().UnprotectedGetEvidence());
		isolatedStorageFile.PostInit();
		return isolatedStorageFile;
	}

	[IsolatedStorageFilePermission(SecurityAction.Demand, UsageAllowed = IsolatedStorageContainment.ApplicationIsolationByUser)]
	public static IsolatedStorageFile GetUserStoreForApplication()
	{
		IsolatedStorageScope scope = IsolatedStorageScope.User | IsolatedStorageScope.Application;
		IsolatedStorageFile isolatedStorageFile = new IsolatedStorageFile(scope);
		isolatedStorageFile.InitStore(scope, null);
		isolatedStorageFile._fullEvidences = Assembly.GetCallingAssembly().UnprotectedGetEvidence();
		isolatedStorageFile.PostInit();
		return isolatedStorageFile;
	}

	[IsolatedStorageFilePermission(SecurityAction.Demand, UsageAllowed = IsolatedStorageContainment.AssemblyIsolationByUser)]
	public static IsolatedStorageFile GetUserStoreForAssembly()
	{
		IsolatedStorageFile isolatedStorageFile = new IsolatedStorageFile(IsolatedStorageScope.User | IsolatedStorageScope.Assembly);
		isolatedStorageFile._assemblyIdentity = GetAssemblyIdentityFromEvidence(isolatedStorageFile._fullEvidences = Assembly.GetCallingAssembly().UnprotectedGetEvidence());
		isolatedStorageFile.PostInit();
		return isolatedStorageFile;
	}

	[IsolatedStorageFilePermission(SecurityAction.Demand, UsageAllowed = IsolatedStorageContainment.DomainIsolationByUser)]
	public static IsolatedStorageFile GetUserStoreForDomain()
	{
		IsolatedStorageFile isolatedStorageFile = new IsolatedStorageFile(IsolatedStorageScope.User | IsolatedStorageScope.Domain | IsolatedStorageScope.Assembly);
		isolatedStorageFile._domainIdentity = GetDomainIdentityFromEvidence(AppDomain.CurrentDomain.Evidence);
		isolatedStorageFile._assemblyIdentity = GetAssemblyIdentityFromEvidence(isolatedStorageFile._fullEvidences = Assembly.GetCallingAssembly().UnprotectedGetEvidence());
		isolatedStorageFile.PostInit();
		return isolatedStorageFile;
	}

	[ComVisible(false)]
	public static IsolatedStorageFile GetUserStoreForSite()
	{
		throw new NotSupportedException();
	}

	public static void Remove(IsolatedStorageScope scope)
	{
		string isolatedStorageRoot = GetIsolatedStorageRoot(scope);
		if (!Directory.Exists(isolatedStorageRoot))
		{
			return;
		}
		try
		{
			Directory.Delete(isolatedStorageRoot, recursive: true);
		}
		catch (IOException)
		{
			throw new IsolatedStorageException("Could not remove storage.");
		}
	}

	internal static string GetIsolatedStorageRoot(IsolatedStorageScope scope)
	{
		string text = null;
		if ((scope & IsolatedStorageScope.User) != 0)
		{
			text = (((scope & IsolatedStorageScope.Roaming) == 0) ? Environment.UnixGetFolderPath(Environment.SpecialFolder.ApplicationData, Environment.SpecialFolderOption.Create) : Environment.UnixGetFolderPath(Environment.SpecialFolder.LocalApplicationData, Environment.SpecialFolderOption.Create));
		}
		else if ((scope & IsolatedStorageScope.Machine) != 0)
		{
			text = Environment.UnixGetFolderPath(Environment.SpecialFolder.CommonApplicationData, Environment.SpecialFolderOption.Create);
		}
		if (text == null)
		{
			throw new IsolatedStorageException(string.Format(Locale.GetText("Couldn't access storage location for '{0}'."), scope));
		}
		return Path.Combine(text, ".isolated-storage");
	}

	private static void Demand(IsolatedStorageScope scope)
	{
		if (SecurityManager.SecurityEnabled)
		{
			IsolatedStorageFilePermission isolatedStorageFilePermission = new IsolatedStorageFilePermission(PermissionState.None);
			isolatedStorageFilePermission.UsageAllowed = ScopeToContainment(scope);
			isolatedStorageFilePermission.Demand();
		}
	}

	private static IsolatedStorageContainment ScopeToContainment(IsolatedStorageScope scope)
	{
		return scope switch
		{
			IsolatedStorageScope.User | IsolatedStorageScope.Domain | IsolatedStorageScope.Assembly => IsolatedStorageContainment.DomainIsolationByUser, 
			IsolatedStorageScope.User | IsolatedStorageScope.Assembly => IsolatedStorageContainment.AssemblyIsolationByUser, 
			IsolatedStorageScope.User | IsolatedStorageScope.Domain | IsolatedStorageScope.Assembly | IsolatedStorageScope.Roaming => IsolatedStorageContainment.DomainIsolationByRoamingUser, 
			IsolatedStorageScope.User | IsolatedStorageScope.Assembly | IsolatedStorageScope.Roaming => IsolatedStorageContainment.AssemblyIsolationByRoamingUser, 
			IsolatedStorageScope.User | IsolatedStorageScope.Application => IsolatedStorageContainment.ApplicationIsolationByUser, 
			IsolatedStorageScope.Domain | IsolatedStorageScope.Assembly | IsolatedStorageScope.Machine => IsolatedStorageContainment.DomainIsolationByMachine, 
			IsolatedStorageScope.Assembly | IsolatedStorageScope.Machine => IsolatedStorageContainment.AssemblyIsolationByMachine, 
			IsolatedStorageScope.Machine | IsolatedStorageScope.Application => IsolatedStorageContainment.ApplicationIsolationByMachine, 
			IsolatedStorageScope.User | IsolatedStorageScope.Roaming | IsolatedStorageScope.Application => IsolatedStorageContainment.ApplicationIsolationByRoamingUser, 
			_ => IsolatedStorageContainment.UnrestrictedIsolatedStorage, 
		};
	}

	internal static ulong GetDirectorySize(DirectoryInfo di)
	{
		ulong num = 0uL;
		FileInfo[] files = di.GetFiles();
		foreach (FileInfo fileInfo in files)
		{
			num += (ulong)fileInfo.Length;
		}
		DirectoryInfo[] directories = di.GetDirectories();
		foreach (DirectoryInfo di2 in directories)
		{
			num += GetDirectorySize(di2);
		}
		return num;
	}

	private IsolatedStorageFile(IsolatedStorageScope scope)
	{
		storage_scope = scope;
	}

	internal IsolatedStorageFile(IsolatedStorageScope scope, string location)
	{
		storage_scope = scope;
		directory = new DirectoryInfo(location);
		if (!directory.Exists)
		{
			throw new IsolatedStorageException(Locale.GetText("Invalid storage."));
		}
	}

	~IsolatedStorageFile()
	{
	}

	private void PostInit()
	{
		string isolatedStorageRoot = GetIsolatedStorageRoot(base.Scope);
		string text = null;
		if (_applicationIdentity != null)
		{
			text = $"a{SeparatorInternal}{GetNameFromIdentity(_applicationIdentity)}";
		}
		else if (_domainIdentity != null)
		{
			text = string.Format("d{0}{1}{0}{2}", SeparatorInternal, GetNameFromIdentity(_domainIdentity), GetNameFromIdentity(_assemblyIdentity));
		}
		else
		{
			if (_assemblyIdentity == null)
			{
				throw new IsolatedStorageException(Locale.GetText("No code identity available."));
			}
			text = string.Format("d{0}none{0}{1}", SeparatorInternal, GetNameFromIdentity(_assemblyIdentity));
		}
		isolatedStorageRoot = Path.Combine(isolatedStorageRoot, text);
		directory = new DirectoryInfo(isolatedStorageRoot);
		if (!directory.Exists)
		{
			try
			{
				directory.Create();
				SaveIdentities(isolatedStorageRoot);
			}
			catch (IOException)
			{
			}
		}
	}

	public void Close()
	{
		closed = true;
	}

	public void CreateDirectory(string dir)
	{
		if (dir == null)
		{
			throw new ArgumentNullException("dir");
		}
		if (dir.IndexOfAny(Path.PathSeparatorChars) < 0)
		{
			if (directory.GetFiles(dir).Length != 0)
			{
				throw new IsolatedStorageException("Unable to create directory.");
			}
			directory.CreateSubdirectory(dir);
			return;
		}
		string[] array = dir.Split(Path.PathSeparatorChars, StringSplitOptions.RemoveEmptyEntries);
		DirectoryInfo directoryInfo = directory;
		for (int i = 0; i < array.Length; i++)
		{
			if (directoryInfo.GetFiles(array[i]).Length != 0)
			{
				throw new IsolatedStorageException("Unable to create directory.");
			}
			directoryInfo = directoryInfo.CreateSubdirectory(array[i]);
		}
	}

	[ComVisible(false)]
	public void CopyFile(string sourceFileName, string destinationFileName)
	{
		CopyFile(sourceFileName, destinationFileName, overwrite: false);
	}

	[ComVisible(false)]
	public void CopyFile(string sourceFileName, string destinationFileName, bool overwrite)
	{
		if (sourceFileName == null)
		{
			throw new ArgumentNullException("sourceFileName");
		}
		if (destinationFileName == null)
		{
			throw new ArgumentNullException("destinationFileName");
		}
		if (sourceFileName.Trim().Length == 0)
		{
			throw new ArgumentException("An empty file name is not valid.", "sourceFileName");
		}
		if (destinationFileName.Trim().Length == 0)
		{
			throw new ArgumentException("An empty file name is not valid.", "destinationFileName");
		}
		CheckOpen();
		string text = Path.Combine(directory.FullName, sourceFileName);
		string text2 = Path.Combine(directory.FullName, destinationFileName);
		if (!IsPathInStorage(text) || !IsPathInStorage(text2))
		{
			throw new IsolatedStorageException("Operation not allowed.");
		}
		if (!Directory.Exists(Path.GetDirectoryName(text)))
		{
			throw new DirectoryNotFoundException("Could not find a part of path '" + sourceFileName + "'.");
		}
		if (!File.Exists(text))
		{
			throw new FileNotFoundException("Could not find a part of path '" + sourceFileName + "'.");
		}
		if (File.Exists(text2) && !overwrite)
		{
			throw new IsolatedStorageException("Operation not allowed.");
		}
		try
		{
			File.Copy(text, text2, overwrite);
		}
		catch (IOException)
		{
			throw new IsolatedStorageException("Operation not allowed.");
		}
	}

	[ComVisible(false)]
	public IsolatedStorageFileStream CreateFile(string path)
	{
		return new IsolatedStorageFileStream(path, FileMode.Create, FileAccess.ReadWrite, FileShare.None, this);
	}

	public void DeleteDirectory(string dir)
	{
		try
		{
			if (Path.IsPathRooted(dir))
			{
				dir = dir.Substring(1);
			}
			directory.CreateSubdirectory(dir).Delete();
		}
		catch
		{
			throw new IsolatedStorageException(Locale.GetText("Could not delete directory '{0}'", dir));
		}
	}

	public void DeleteFile(string file)
	{
		if (file == null)
		{
			throw new ArgumentNullException("file");
		}
		if (!File.Exists(Path.Combine(directory.FullName, file)))
		{
			throw new IsolatedStorageException(Locale.GetText("Could not delete file '{0}'", file));
		}
		try
		{
			File.Delete(Path.Combine(directory.FullName, file));
		}
		catch
		{
			throw new IsolatedStorageException(Locale.GetText("Could not delete file '{0}'", file));
		}
	}

	public void Dispose()
	{
		disposed = true;
		GC.SuppressFinalize(this);
	}

	[ComVisible(false)]
	public bool DirectoryExists(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		CheckOpen();
		string path2 = Path.Combine(directory.FullName, path);
		if (!IsPathInStorage(path2))
		{
			return false;
		}
		return Directory.Exists(path2);
	}

	[ComVisible(false)]
	public bool FileExists(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		CheckOpen();
		string path2 = Path.Combine(directory.FullName, path);
		if (!IsPathInStorage(path2))
		{
			return false;
		}
		return File.Exists(path2);
	}

	[ComVisible(false)]
	public DateTimeOffset GetCreationTime(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (path.Trim().Length == 0)
		{
			throw new ArgumentException("An empty path is not valid.");
		}
		CheckOpen();
		string path2 = Path.Combine(directory.FullName, path);
		if (File.Exists(path2))
		{
			return File.GetCreationTime(path2);
		}
		return Directory.GetCreationTime(path2);
	}

	[ComVisible(false)]
	public DateTimeOffset GetLastAccessTime(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (path.Trim().Length == 0)
		{
			throw new ArgumentException("An empty path is not valid.");
		}
		CheckOpen();
		string path2 = Path.Combine(directory.FullName, path);
		if (File.Exists(path2))
		{
			return File.GetLastAccessTime(path2);
		}
		return Directory.GetLastAccessTime(path2);
	}

	[ComVisible(false)]
	public DateTimeOffset GetLastWriteTime(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (path.Trim().Length == 0)
		{
			throw new ArgumentException("An empty path is not valid.");
		}
		CheckOpen();
		string path2 = Path.Combine(directory.FullName, path);
		if (File.Exists(path2))
		{
			return File.GetLastWriteTime(path2);
		}
		return Directory.GetLastWriteTime(path2);
	}

	public string[] GetDirectoryNames(string searchPattern)
	{
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		if (searchPattern.Contains(".."))
		{
			throw new ArgumentException("Search pattern cannot contain '..' to move up directories.", "searchPattern");
		}
		string directoryName = Path.GetDirectoryName(searchPattern);
		string fileName = Path.GetFileName(searchPattern);
		DirectoryInfo[] array = null;
		if (directoryName == null || directoryName.Length == 0)
		{
			array = directory.GetDirectories(searchPattern);
		}
		else
		{
			DirectoryInfo directoryInfo = directory.GetDirectories(directoryName)[0];
			if (directoryInfo.FullName.IndexOf(directory.FullName) >= 0)
			{
				array = directoryInfo.GetDirectories(fileName);
				string[] array2 = directoryName.Split(new char[1] { Path.DirectorySeparatorChar }, StringSplitOptions.RemoveEmptyEntries);
				for (int num = array2.Length - 1; num >= 0; num--)
				{
					if (directoryInfo.Name != array2[num])
					{
						array = null;
						break;
					}
					directoryInfo = directoryInfo.Parent;
				}
			}
		}
		if (array == null)
		{
			throw new SecurityException();
		}
		return GetNames(array);
	}

	[ComVisible(false)]
	public string[] GetDirectoryNames()
	{
		return GetDirectoryNames("*");
	}

	private string[] GetNames(FileSystemInfo[] afsi)
	{
		string[] array = new string[afsi.Length];
		for (int i = 0; i != afsi.Length; i++)
		{
			array[i] = afsi[i].Name;
		}
		return array;
	}

	public string[] GetFileNames(string searchPattern)
	{
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		if (searchPattern.Contains(".."))
		{
			throw new ArgumentException("Search pattern cannot contain '..' to move up directories.", "searchPattern");
		}
		string directoryName = Path.GetDirectoryName(searchPattern);
		string fileName = Path.GetFileName(searchPattern);
		FileInfo[] array = null;
		if (directoryName == null || directoryName.Length == 0)
		{
			array = directory.GetFiles(searchPattern);
		}
		else
		{
			DirectoryInfo[] directories = directory.GetDirectories(directoryName);
			if (directories.Length != 1 || !(directories[0].Name == directoryName) || directories[0].FullName.IndexOf(directory.FullName) < 0)
			{
				throw new SecurityException();
			}
			array = directories[0].GetFiles(fileName);
		}
		return GetNames(array);
	}

	[ComVisible(false)]
	public string[] GetFileNames()
	{
		return GetFileNames("*");
	}

	[ComVisible(false)]
	public override bool IncreaseQuotaTo(long newQuotaSize)
	{
		if (newQuotaSize < Quota)
		{
			throw new ArgumentException();
		}
		CheckOpen();
		return false;
	}

	[ComVisible(false)]
	public void MoveDirectory(string sourceDirectoryName, string destinationDirectoryName)
	{
		if (sourceDirectoryName == null)
		{
			throw new ArgumentNullException("sourceDirectoryName");
		}
		if (destinationDirectoryName == null)
		{
			throw new ArgumentNullException("sourceDirectoryName");
		}
		if (sourceDirectoryName.Trim().Length == 0)
		{
			throw new ArgumentException("An empty directory name is not valid.", "sourceDirectoryName");
		}
		if (destinationDirectoryName.Trim().Length == 0)
		{
			throw new ArgumentException("An empty directory name is not valid.", "destinationDirectoryName");
		}
		CheckOpen();
		string text = Path.Combine(directory.FullName, sourceDirectoryName);
		string text2 = Path.Combine(directory.FullName, destinationDirectoryName);
		if (!IsPathInStorage(text) || !IsPathInStorage(text2))
		{
			throw new IsolatedStorageException("Operation not allowed.");
		}
		if (!Directory.Exists(text))
		{
			throw new DirectoryNotFoundException("Could not find a part of path '" + sourceDirectoryName + "'.");
		}
		if (!Directory.Exists(Path.GetDirectoryName(text2)))
		{
			throw new DirectoryNotFoundException("Could not find a part of path '" + destinationDirectoryName + "'.");
		}
		try
		{
			Directory.Move(text, text2);
		}
		catch (IOException)
		{
			throw new IsolatedStorageException("Operation not allowed.");
		}
	}

	[ComVisible(false)]
	public void MoveFile(string sourceFileName, string destinationFileName)
	{
		if (sourceFileName == null)
		{
			throw new ArgumentNullException("sourceFileName");
		}
		if (destinationFileName == null)
		{
			throw new ArgumentNullException("sourceFileName");
		}
		if (sourceFileName.Trim().Length == 0)
		{
			throw new ArgumentException("An empty file name is not valid.", "sourceFileName");
		}
		if (destinationFileName.Trim().Length == 0)
		{
			throw new ArgumentException("An empty file name is not valid.", "destinationFileName");
		}
		CheckOpen();
		string text = Path.Combine(directory.FullName, sourceFileName);
		string text2 = Path.Combine(directory.FullName, destinationFileName);
		if (!IsPathInStorage(text) || !IsPathInStorage(text2))
		{
			throw new IsolatedStorageException("Operation not allowed.");
		}
		if (!File.Exists(text))
		{
			throw new FileNotFoundException("Could not find a part of path '" + sourceFileName + "'.");
		}
		if (!Directory.Exists(Path.GetDirectoryName(text2)))
		{
			throw new IsolatedStorageException("Operation not allowed.");
		}
		try
		{
			File.Move(text, text2);
		}
		catch (IOException)
		{
			throw new IsolatedStorageException("Operation not allowed.");
		}
	}

	[ComVisible(false)]
	public IsolatedStorageFileStream OpenFile(string path, FileMode mode)
	{
		return new IsolatedStorageFileStream(path, mode, this);
	}

	[ComVisible(false)]
	public IsolatedStorageFileStream OpenFile(string path, FileMode mode, FileAccess access)
	{
		return new IsolatedStorageFileStream(path, mode, access, this);
	}

	[ComVisible(false)]
	public IsolatedStorageFileStream OpenFile(string path, FileMode mode, FileAccess access, FileShare share)
	{
		return new IsolatedStorageFileStream(path, mode, access, share, this);
	}

	public override void Remove()
	{
		CheckOpen(checkDirExists: false);
		try
		{
			directory.Delete(recursive: true);
		}
		catch
		{
			throw new IsolatedStorageException("Could not remove storage.");
		}
		Close();
	}

	protected override IsolatedStoragePermission GetPermission(PermissionSet ps)
	{
		if (ps == null)
		{
			return null;
		}
		return (IsolatedStoragePermission)ps.GetPermission(typeof(IsolatedStorageFilePermission));
	}

	private void CheckOpen()
	{
		CheckOpen(checkDirExists: true);
	}

	private void CheckOpen(bool checkDirExists)
	{
		if (disposed)
		{
			throw new ObjectDisposedException("IsolatedStorageFile");
		}
		if (closed)
		{
			throw new InvalidOperationException("Storage needs to be open for this operation.");
		}
		if (checkDirExists && !Directory.Exists(directory.FullName))
		{
			throw new IsolatedStorageException("Isolated storage has been removed or disabled.");
		}
	}

	private bool IsPathInStorage(string path)
	{
		return Path.GetFullPath(path).StartsWith(directory.FullName);
	}

	private string GetNameFromIdentity(object identity)
	{
		byte[] bytes = Encoding.UTF8.GetBytes(identity.ToString());
		byte[] src = SHA1.Create().ComputeHash(bytes, 0, bytes.Length);
		byte[] array = new byte[10];
		Buffer.BlockCopy(src, 0, array, 0, array.Length);
		return CryptoConvert.ToHex(array);
	}

	private static object GetTypeFromEvidence(Evidence e, Type t)
	{
		foreach (object item in e)
		{
			if (item.GetType() == t)
			{
				return item;
			}
		}
		return null;
	}

	internal static object GetAssemblyIdentityFromEvidence(Evidence e)
	{
		object typeFromEvidence = GetTypeFromEvidence(e, typeof(Publisher));
		if (typeFromEvidence != null)
		{
			return typeFromEvidence;
		}
		typeFromEvidence = GetTypeFromEvidence(e, typeof(StrongName));
		if (typeFromEvidence != null)
		{
			return typeFromEvidence;
		}
		return GetTypeFromEvidence(e, typeof(Url));
	}

	internal static object GetDomainIdentityFromEvidence(Evidence e)
	{
		object typeFromEvidence = GetTypeFromEvidence(e, typeof(ApplicationDirectory));
		if (typeFromEvidence != null)
		{
			return typeFromEvidence;
		}
		return GetTypeFromEvidence(e, typeof(Url));
	}

	[SecurityPermission(SecurityAction.Assert, SerializationFormatter = true)]
	private void SaveIdentities(string root)
	{
		Identities identities = new Identities(_applicationIdentity, _assemblyIdentity, _domainIdentity);
		BinaryFormatter binaryFormatter = new BinaryFormatter();
		mutex.WaitOne();
		try
		{
			using FileStream serializationStream = File.Create(root + ".storage");
			binaryFormatter.Serialize(serializationStream, identities);
		}
		finally
		{
			mutex.ReleaseMutex();
		}
	}

	internal IsolatedStorageFile()
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
