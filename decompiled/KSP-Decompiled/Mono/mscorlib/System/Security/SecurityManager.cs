using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Security.Policy;
using System.Text;

namespace System.Security;

[ComVisible(true)]
public static class SecurityManager
{
	private static object _lockObject;

	private static ArrayList _hierarchy;

	private static IPermission _unmanagedCode;

	private static Hashtable _declsecCache;

	private static PolicyLevel _level;

	private static SecurityPermission _execution;

	[Obsolete]
	public static bool CheckExecutionRights
	{
		get
		{
			return false;
		}
		set
		{
		}
	}

	[Obsolete("The security manager cannot be turned off on MS runtime")]
	public static extern bool SecurityEnabled
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		[SecurityPermission(SecurityAction.Demand, ControlPolicy = true)]
		set;
	}

	private static IEnumerator Hierarchy
	{
		get
		{
			lock (_lockObject)
			{
				if (_hierarchy == null)
				{
					InitializePolicyHierarchy();
				}
			}
			return _hierarchy.GetEnumerator();
		}
	}

	internal static PolicyLevel ResolvingPolicyLevel
	{
		get
		{
			return _level;
		}
		set
		{
			_level = value;
		}
	}

	private static IPermission UnmanagedCode
	{
		get
		{
			lock (_lockObject)
			{
				if (_unmanagedCode == null)
				{
					_unmanagedCode = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
				}
			}
			return _unmanagedCode;
		}
	}

	static SecurityManager()
	{
		_execution = new SecurityPermission(SecurityPermissionFlag.Execution);
		_lockObject = new object();
	}

	internal static bool CheckElevatedPermissions()
	{
		return true;
	}

	[Conditional("ENABLE_SANDBOX")]
	internal static void EnsureElevatedPermissions()
	{
	}

	[MonoTODO("CAS support is experimental (and unsupported). This method only works in FullTrust.")]
	[StrongNameIdentityPermission(SecurityAction.LinkDemand, PublicKey = "0x00000000000000000400000000000000")]
	public static void GetZoneAndOrigin(out ArrayList zone, out ArrayList origin)
	{
		zone = new ArrayList();
		origin = new ArrayList();
	}

	[Obsolete]
	public static bool IsGranted(IPermission perm)
	{
		if (perm == null)
		{
			return true;
		}
		if (!SecurityEnabled)
		{
			return true;
		}
		return IsGranted(Assembly.GetCallingAssembly(), perm);
	}

	internal static bool IsGranted(Assembly a, IPermission perm)
	{
		PermissionSet grantedPermissionSet = a.GrantedPermissionSet;
		if (grantedPermissionSet != null && !grantedPermissionSet.IsUnrestricted())
		{
			CodeAccessPermission target = (CodeAccessPermission)grantedPermissionSet.GetPermission(perm.GetType());
			if (!perm.IsSubsetOf(target))
			{
				return false;
			}
		}
		PermissionSet deniedPermissionSet = a.DeniedPermissionSet;
		if (deniedPermissionSet != null && !deniedPermissionSet.IsEmpty())
		{
			if (deniedPermissionSet.IsUnrestricted())
			{
				return false;
			}
			CodeAccessPermission codeAccessPermission = (CodeAccessPermission)a.DeniedPermissionSet.GetPermission(perm.GetType());
			if (codeAccessPermission != null && perm.IsSubsetOf(codeAccessPermission))
			{
				return false;
			}
		}
		return true;
	}

	[Obsolete]
	[SecurityPermission(SecurityAction.Demand, ControlPolicy = true)]
	public static PolicyLevel LoadPolicyLevelFromFile(string path, PolicyLevelType type)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		PolicyLevel policyLevel = null;
		try
		{
			policyLevel = new PolicyLevel(type.ToString(), type);
			policyLevel.LoadFromFile(path);
			return policyLevel;
		}
		catch (Exception innerException)
		{
			throw new ArgumentException(Locale.GetText("Invalid policy XML"), innerException);
		}
	}

	[Obsolete]
	[SecurityPermission(SecurityAction.Demand, ControlPolicy = true)]
	public static PolicyLevel LoadPolicyLevelFromString(string str, PolicyLevelType type)
	{
		if (str == null)
		{
			throw new ArgumentNullException("str");
		}
		PolicyLevel policyLevel = null;
		try
		{
			policyLevel = new PolicyLevel(type.ToString(), type);
			policyLevel.LoadFromString(str);
			return policyLevel;
		}
		catch (Exception innerException)
		{
			throw new ArgumentException(Locale.GetText("Invalid policy XML"), innerException);
		}
	}

	[Obsolete]
	[SecurityPermission(SecurityAction.Demand, ControlPolicy = true)]
	public static IEnumerator PolicyHierarchy()
	{
		return Hierarchy;
	}

	[Obsolete]
	public static PermissionSet ResolvePolicy(Evidence evidence)
	{
		if (evidence == null)
		{
			return new PermissionSet(PermissionState.None);
		}
		PermissionSet ps = null;
		IEnumerator hierarchy = Hierarchy;
		while (hierarchy.MoveNext())
		{
			PolicyLevel pl = (PolicyLevel)hierarchy.Current;
			if (ResolvePolicyLevel(ref ps, pl, evidence))
			{
				break;
			}
		}
		ResolveIdentityPermissions(ps, evidence);
		return ps;
	}

	[Obsolete]
	[MonoTODO("(2.0) more tests are needed")]
	public static PermissionSet ResolvePolicy(Evidence[] evidences)
	{
		if (evidences == null || evidences.Length == 0 || (evidences.Length == 1 && evidences[0].Count == 0))
		{
			return new PermissionSet(PermissionState.None);
		}
		PermissionSet permissionSet = ResolvePolicy(evidences[0]);
		for (int i = 1; i < evidences.Length; i++)
		{
			permissionSet = permissionSet.Intersect(ResolvePolicy(evidences[i]));
		}
		return permissionSet;
	}

	[Obsolete]
	public static PermissionSet ResolveSystemPolicy(Evidence evidence)
	{
		if (evidence == null)
		{
			return new PermissionSet(PermissionState.None);
		}
		PermissionSet ps = null;
		IEnumerator hierarchy = Hierarchy;
		while (hierarchy.MoveNext())
		{
			PolicyLevel policyLevel = (PolicyLevel)hierarchy.Current;
			if (policyLevel.Type == PolicyLevelType.AppDomain || ResolvePolicyLevel(ref ps, policyLevel, evidence))
			{
				break;
			}
		}
		ResolveIdentityPermissions(ps, evidence);
		return ps;
	}

	[Obsolete]
	public static PermissionSet ResolvePolicy(Evidence evidence, PermissionSet reqdPset, PermissionSet optPset, PermissionSet denyPset, out PermissionSet denied)
	{
		PermissionSet permissionSet = ResolvePolicy(evidence);
		if (reqdPset != null && !reqdPset.IsSubsetOf(permissionSet))
		{
			throw new PolicyException(Locale.GetText("Policy doesn't grant the minimal permissions required to execute the assembly."));
		}
		if (CheckExecutionRights)
		{
			bool flag = false;
			if (permissionSet != null)
			{
				if (permissionSet.IsUnrestricted())
				{
					flag = true;
				}
				else
				{
					IPermission permission = permissionSet.GetPermission(typeof(SecurityPermission));
					flag = _execution.IsSubsetOf(permission);
				}
			}
			if (!flag)
			{
				throw new PolicyException(Locale.GetText("Policy doesn't grant the right to execute the assembly."));
			}
		}
		denied = denyPset;
		return permissionSet;
	}

	[Obsolete]
	public static IEnumerator ResolvePolicyGroups(Evidence evidence)
	{
		if (evidence == null)
		{
			throw new ArgumentNullException("evidence");
		}
		ArrayList arrayList = new ArrayList();
		IEnumerator hierarchy = Hierarchy;
		while (hierarchy.MoveNext())
		{
			CodeGroup value = ((PolicyLevel)hierarchy.Current).ResolveMatchingCodeGroups(evidence);
			arrayList.Add(value);
		}
		return arrayList.GetEnumerator();
	}

	[Obsolete]
	[SecurityPermission(SecurityAction.Demand, ControlPolicy = true)]
	public static void SavePolicy()
	{
		IEnumerator hierarchy = Hierarchy;
		while (hierarchy.MoveNext())
		{
			(hierarchy.Current as PolicyLevel).Save();
		}
	}

	[Obsolete]
	[SecurityPermission(SecurityAction.Demand, ControlPolicy = true)]
	public static void SavePolicyLevel(PolicyLevel level)
	{
		level.Save();
	}

	private static void InitializePolicyHierarchy()
	{
		string directoryName = Path.GetDirectoryName(Environment.GetMachineConfigPath());
		string path = Path.Combine(Environment.UnixGetFolderPath(Environment.SpecialFolder.ApplicationData, Environment.SpecialFolderOption.Create), "mono");
		PolicyLevel policyLevel = (_level = new PolicyLevel("Enterprise", PolicyLevelType.Enterprise));
		policyLevel.LoadFromFile(Path.Combine(directoryName, "enterprisesec.config"));
		PolicyLevel policyLevel2 = (_level = new PolicyLevel("Machine", PolicyLevelType.Machine));
		policyLevel2.LoadFromFile(Path.Combine(directoryName, "security.config"));
		PolicyLevel policyLevel3 = (_level = new PolicyLevel("User", PolicyLevelType.User));
		policyLevel3.LoadFromFile(Path.Combine(path, "security.config"));
		_hierarchy = ArrayList.Synchronized(new ArrayList { policyLevel, policyLevel2, policyLevel3 });
		_level = null;
	}

	internal static bool ResolvePolicyLevel(ref PermissionSet ps, PolicyLevel pl, Evidence evidence)
	{
		PolicyStatement policyStatement = pl.Resolve(evidence);
		if (policyStatement != null)
		{
			if (ps == null)
			{
				ps = policyStatement.PermissionSet;
			}
			else
			{
				ps = ps.Intersect(policyStatement.PermissionSet);
				if (ps == null)
				{
					ps = new PermissionSet(PermissionState.None);
				}
			}
			if ((policyStatement.Attributes & PolicyStatementAttribute.LevelFinal) == PolicyStatementAttribute.LevelFinal)
			{
				return true;
			}
		}
		return false;
	}

	internal static void ResolveIdentityPermissions(PermissionSet ps, Evidence evidence)
	{
		if (ps.IsUnrestricted())
		{
			return;
		}
		IEnumerator hostEnumerator = evidence.GetHostEnumerator();
		while (hostEnumerator.MoveNext())
		{
			if (hostEnumerator.Current is IIdentityPermissionFactory identityPermissionFactory)
			{
				IPermission perm = identityPermissionFactory.CreateIdentityPermission(evidence);
				ps.AddPermission(perm);
			}
		}
	}

	internal static PermissionSet Decode(IntPtr permissions, int length)
	{
		PermissionSet permissionSet = null;
		lock (_lockObject)
		{
			if (_declsecCache == null)
			{
				_declsecCache = new Hashtable();
			}
			object key = (int)permissions;
			permissionSet = (PermissionSet)_declsecCache[key];
			if (permissionSet == null)
			{
				byte[] array = new byte[length];
				Marshal.Copy(permissions, array, 0, length);
				permissionSet = Decode(array);
				permissionSet.DeclarativeSecurity = true;
				_declsecCache.Add(key, permissionSet);
			}
		}
		return permissionSet;
	}

	internal static PermissionSet Decode(byte[] encodedPermissions)
	{
		if (encodedPermissions == null || encodedPermissions.Length < 1)
		{
			throw new SecurityException("Invalid metadata format.");
		}
		return encodedPermissions[0] switch
		{
			60 => new PermissionSet(Encoding.Unicode.GetString(encodedPermissions)), 
			46 => PermissionSet.CreateFromBinaryFormat(encodedPermissions), 
			_ => throw new SecurityException(Locale.GetText("Unknown metadata format.")), 
		};
	}

	private static void ThrowException(Exception ex)
	{
		throw ex;
	}

	public static PermissionSet GetStandardSandbox(Evidence evidence)
	{
		if (evidence == null)
		{
			throw new ArgumentNullException("evidence");
		}
		throw new NotImplementedException();
	}

	public static bool CurrentThreadRequiresSecurityContextCapture()
	{
		throw new NotImplementedException();
	}
}
