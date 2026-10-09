using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;

namespace System.IO.IsolatedStorage;

[ComVisible(true)]
public abstract class IsolatedStorage : MarshalByRefObject
{
	internal IsolatedStorageScope storage_scope;

	internal object _assemblyIdentity;

	internal object _domainIdentity;

	internal object _applicationIdentity;

	[ComVisible(false)]
	[MonoTODO("Does not currently use the manifest support")]
	public object ApplicationIdentity
	{
		[SecurityPermission(SecurityAction.Demand, ControlPolicy = true)]
		get
		{
			if ((storage_scope & IsolatedStorageScope.Application) == 0)
			{
				throw new InvalidOperationException(Locale.GetText("Invalid Isolation Scope."));
			}
			if (_applicationIdentity == null)
			{
				throw new InvalidOperationException(Locale.GetText("Identity unavailable."));
			}
			throw new NotImplementedException(Locale.GetText("CAS related"));
		}
	}

	public object AssemblyIdentity
	{
		[SecurityPermission(SecurityAction.Demand, ControlPolicy = true)]
		get
		{
			if ((storage_scope & IsolatedStorageScope.Assembly) == 0)
			{
				throw new InvalidOperationException(Locale.GetText("Invalid Isolation Scope."));
			}
			if (_assemblyIdentity == null)
			{
				throw new InvalidOperationException(Locale.GetText("Identity unavailable."));
			}
			return _assemblyIdentity;
		}
	}

	[Obsolete]
	[CLSCompliant(false)]
	public virtual ulong CurrentSize
	{
		get
		{
			throw new InvalidOperationException(Locale.GetText("IsolatedStorage does not have a preset CurrentSize."));
		}
	}

	public object DomainIdentity
	{
		[SecurityPermission(SecurityAction.Demand, ControlPolicy = true)]
		get
		{
			if ((storage_scope & IsolatedStorageScope.Domain) == 0)
			{
				throw new InvalidOperationException(Locale.GetText("Invalid Isolation Scope."));
			}
			if (_domainIdentity == null)
			{
				throw new InvalidOperationException(Locale.GetText("Identity unavailable."));
			}
			return _domainIdentity;
		}
	}

	[Obsolete]
	[CLSCompliant(false)]
	public virtual ulong MaximumSize
	{
		get
		{
			throw new InvalidOperationException(Locale.GetText("IsolatedStorage does not have a preset MaximumSize."));
		}
	}

	public IsolatedStorageScope Scope => storage_scope;

	[ComVisible(false)]
	public virtual long AvailableFreeSpace
	{
		get
		{
			throw new InvalidOperationException("This property is not defined for this store.");
		}
	}

	[ComVisible(false)]
	public virtual long Quota
	{
		get
		{
			throw new InvalidOperationException("This property is not defined for this store.");
		}
	}

	[ComVisible(false)]
	public virtual long UsedSize
	{
		get
		{
			throw new InvalidOperationException("This property is not defined for this store.");
		}
	}

	protected virtual char SeparatorExternal => Path.DirectorySeparatorChar;

	protected virtual char SeparatorInternal => '.';

	protected abstract IsolatedStoragePermission GetPermission(PermissionSet ps);

	protected void InitStore(IsolatedStorageScope scope, Type domainEvidenceType, Type assemblyEvidenceType)
	{
		if (scope == (IsolatedStorageScope.User | IsolatedStorageScope.Assembly) || scope == (IsolatedStorageScope.User | IsolatedStorageScope.Domain | IsolatedStorageScope.Assembly))
		{
			throw new NotImplementedException(scope.ToString());
		}
		throw new ArgumentException(scope.ToString());
	}

	[MonoTODO("requires manifest support")]
	protected void InitStore(IsolatedStorageScope scope, Type appEvidenceType)
	{
		if (AppDomain.CurrentDomain.ApplicationIdentity == null)
		{
			throw new IsolatedStorageException(Locale.GetText("No ApplicationIdentity available for AppDomain."));
		}
		_ = appEvidenceType == null;
		storage_scope = scope;
	}

	public abstract void Remove();

	[ComVisible(false)]
	public virtual bool IncreaseQuotaTo(long newQuotaSize)
	{
		return false;
	}
}
