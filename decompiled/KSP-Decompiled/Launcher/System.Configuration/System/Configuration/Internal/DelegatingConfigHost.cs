using System.IO;
using System.Security;
using System.Xml;
using Unity;

namespace System.Configuration.Internal;

public class DelegatingConfigHost : IInternalConfigHost, IInternalConfigurationBuilderHost
{
	protected IInternalConfigurationBuilderHost ConfigBuilderHost
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected IInternalConfigHost Host
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public virtual bool IsRemote
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public virtual bool SupportsChangeNotifications
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public virtual bool SupportsLocation
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public virtual bool SupportsPath
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public virtual bool SupportsRefresh
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	protected DelegatingConfigHost()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual object CreateConfigurationContext(string configPath, string locationSubPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual object CreateDeprecatedConfigContext(string configPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual string DecryptSection(string encryptedXml, ProtectedConfigurationProvider protectionProvider, ProtectedConfigurationSection protectedConfigSection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual void DeleteStream(string streamName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual string EncryptSection(string clearTextXml, ProtectedConfigurationProvider protectionProvider, ProtectedConfigurationSection protectedConfigSection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual string GetConfigPathFromLocationSubPath(string configPath, string locationSubPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual Type GetConfigType(string typeName, bool throwOnError)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual string GetConfigTypeName(Type t)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual void GetRestrictedPermissions(IInternalConfigRecord configRecord, out PermissionSet permissionSet, out bool isHostReady)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual string GetStreamName(string configPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual string GetStreamNameForConfigSource(string streamName, string configSource)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual object GetStreamVersion(string streamName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual IDisposable Impersonate()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual void Init(IInternalConfigRoot configRoot, object[] hostInitParams)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void InitForConfiguration(ref string locationSubPath, out string configPath, out string locationConfigPath, IInternalConfigRoot configRoot, object[] hostInitConfigurationParams)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual bool IsAboveApplication(string configPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual bool IsConfigRecordRequired(string configPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual bool IsDefinitionAllowed(string configPath, ConfigurationAllowDefinition allowDefinition, ConfigurationAllowExeDefinition allowExeDefinition)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual bool IsFile(string streamName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual bool IsFullTrustSectionWithoutAptcaAllowed(IInternalConfigRecord configRecord)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual bool IsInitDelayed(IInternalConfigRecord configRecord)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual bool IsLocationApplicable(string configPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual bool IsSecondaryRoot(string configPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual bool IsTrustedConfigPath(string configPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual Stream OpenStreamForRead(string streamName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual Stream OpenStreamForRead(string streamName, bool assertPermissions)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual Stream OpenStreamForWrite(string streamName, string templateStreamName, ref object writeContext)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual Stream OpenStreamForWrite(string streamName, string templateStreamName, ref object writeContext, bool assertPermissions)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual bool PrefetchAll(string configPath, string streamName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual bool PrefetchSection(string sectionGroupName, string sectionName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual ConfigurationSection ProcessConfigurationSection(ConfigurationSection configSection, ConfigurationBuilder builder)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual XmlNode ProcessRawXml(XmlNode rawXml, ConfigurationBuilder builder)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual void RequireCompleteInit(IInternalConfigRecord configRecord)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual object StartMonitoringStreamForChanges(string streamName, StreamChangeCallback callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual void StopMonitoringStreamForChanges(string streamName, StreamChangeCallback callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void VerifyDefinitionAllowed(string configPath, ConfigurationAllowDefinition allowDefinition, ConfigurationAllowExeDefinition allowExeDefinition, IConfigErrorInfo errorInfo)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void WriteCompleted(string streamName, bool success, object writeContext)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void WriteCompleted(string streamName, bool success, object writeContext, bool assertPermissions)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
