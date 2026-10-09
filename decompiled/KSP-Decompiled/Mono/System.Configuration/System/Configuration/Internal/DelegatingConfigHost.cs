using System.IO;
using System.Security;
using System.Xml;
using Unity;

namespace System.Configuration.Internal;

public class DelegatingConfigHost : IInternalConfigHost, IInternalConfigurationBuilderHost
{
	private IInternalConfigHost host;

	protected IInternalConfigHost Host
	{
		get
		{
			return host;
		}
		set
		{
			host = value;
		}
	}

	public virtual bool IsRemote => host.IsRemote;

	public virtual bool SupportsChangeNotifications => host.SupportsChangeNotifications;

	public virtual bool SupportsLocation => host.SupportsLocation;

	public virtual bool SupportsPath => host.SupportsPath;

	public virtual bool SupportsRefresh => host.SupportsRefresh;

	protected IInternalConfigurationBuilderHost ConfigBuilderHost
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected DelegatingConfigHost()
	{
	}

	public virtual object CreateConfigurationContext(string configPath, string locationSubPath)
	{
		return host.CreateConfigurationContext(configPath, locationSubPath);
	}

	public virtual object CreateDeprecatedConfigContext(string configPath)
	{
		return host.CreateDeprecatedConfigContext(configPath);
	}

	public virtual string DecryptSection(string encryptedXml, ProtectedConfigurationProvider protectionProvider, ProtectedConfigurationSection protectedConfigSection)
	{
		return host.DecryptSection(encryptedXml, protectionProvider, protectedConfigSection);
	}

	public virtual void DeleteStream(string streamName)
	{
		host.DeleteStream(streamName);
	}

	public virtual string EncryptSection(string clearTextXml, ProtectedConfigurationProvider protectionProvider, ProtectedConfigurationSection protectedConfigSection)
	{
		return host.EncryptSection(clearTextXml, protectionProvider, protectedConfigSection);
	}

	public virtual string GetConfigPathFromLocationSubPath(string configPath, string locationSubPath)
	{
		return host.GetConfigPathFromLocationSubPath(configPath, locationSubPath);
	}

	public virtual Type GetConfigType(string typeName, bool throwOnError)
	{
		return host.GetConfigType(typeName, throwOnError);
	}

	public virtual string GetConfigTypeName(Type t)
	{
		return host.GetConfigTypeName(t);
	}

	public virtual void GetRestrictedPermissions(IInternalConfigRecord configRecord, out PermissionSet permissionSet, out bool isHostReady)
	{
		host.GetRestrictedPermissions(configRecord, out permissionSet, out isHostReady);
	}

	public virtual string GetStreamName(string configPath)
	{
		return host.GetStreamName(configPath);
	}

	public virtual string GetStreamNameForConfigSource(string streamName, string configSource)
	{
		return host.GetStreamNameForConfigSource(streamName, configSource);
	}

	public virtual object GetStreamVersion(string streamName)
	{
		return host.GetStreamVersion(streamName);
	}

	public virtual IDisposable Impersonate()
	{
		return host.Impersonate();
	}

	public virtual void Init(IInternalConfigRoot configRoot, params object[] hostInitParams)
	{
		host.Init(configRoot, hostInitParams);
	}

	public virtual void InitForConfiguration(ref string locationSubPath, out string configPath, out string locationConfigPath, IInternalConfigRoot configRoot, params object[] hostInitConfigurationParams)
	{
		host.InitForConfiguration(ref locationSubPath, out configPath, out locationConfigPath, configRoot, hostInitConfigurationParams);
	}

	public virtual bool IsAboveApplication(string configPath)
	{
		return host.IsAboveApplication(configPath);
	}

	public virtual bool IsConfigRecordRequired(string configPath)
	{
		return host.IsConfigRecordRequired(configPath);
	}

	public virtual bool IsDefinitionAllowed(string configPath, ConfigurationAllowDefinition allowDefinition, ConfigurationAllowExeDefinition allowExeDefinition)
	{
		return host.IsDefinitionAllowed(configPath, allowDefinition, allowExeDefinition);
	}

	public virtual bool IsInitDelayed(IInternalConfigRecord configRecord)
	{
		return host.IsInitDelayed(configRecord);
	}

	public virtual bool IsFile(string streamName)
	{
		return host.IsFile(streamName);
	}

	public virtual bool IsFullTrustSectionWithoutAptcaAllowed(IInternalConfigRecord configRecord)
	{
		return host.IsFullTrustSectionWithoutAptcaAllowed(configRecord);
	}

	public virtual bool IsLocationApplicable(string configPath)
	{
		return host.IsLocationApplicable(configPath);
	}

	public virtual bool IsSecondaryRoot(string configPath)
	{
		return host.IsSecondaryRoot(configPath);
	}

	public virtual bool IsTrustedConfigPath(string configPath)
	{
		return host.IsTrustedConfigPath(configPath);
	}

	public virtual Stream OpenStreamForRead(string streamName)
	{
		return host.OpenStreamForRead(streamName);
	}

	public virtual Stream OpenStreamForRead(string streamName, bool assertPermissions)
	{
		return host.OpenStreamForRead(streamName, assertPermissions);
	}

	public virtual Stream OpenStreamForWrite(string streamName, string templateStreamName, ref object writeContext)
	{
		return host.OpenStreamForWrite(streamName, templateStreamName, ref writeContext);
	}

	public virtual Stream OpenStreamForWrite(string streamName, string templateStreamName, ref object writeContext, bool assertPermissions)
	{
		return host.OpenStreamForWrite(streamName, templateStreamName, ref writeContext, assertPermissions);
	}

	public virtual bool PrefetchAll(string configPath, string streamName)
	{
		return host.PrefetchAll(configPath, streamName);
	}

	public virtual bool PrefetchSection(string sectionGroupName, string sectionName)
	{
		return host.PrefetchSection(sectionGroupName, sectionName);
	}

	public virtual void RequireCompleteInit(IInternalConfigRecord configRecord)
	{
		host.RequireCompleteInit(configRecord);
	}

	public virtual object StartMonitoringStreamForChanges(string streamName, StreamChangeCallback callback)
	{
		return host.StartMonitoringStreamForChanges(streamName, callback);
	}

	public virtual void StopMonitoringStreamForChanges(string streamName, StreamChangeCallback callback)
	{
		host.StopMonitoringStreamForChanges(streamName, callback);
	}

	public virtual void VerifyDefinitionAllowed(string configPath, ConfigurationAllowDefinition allowDefinition, ConfigurationAllowExeDefinition allowExeDefinition, IConfigErrorInfo errorInfo)
	{
		host.VerifyDefinitionAllowed(configPath, allowDefinition, allowExeDefinition, errorInfo);
	}

	public virtual void WriteCompleted(string streamName, bool success, object writeContext)
	{
		host.WriteCompleted(streamName, success, writeContext);
	}

	public virtual void WriteCompleted(string streamName, bool success, object writeContext, bool assertPermissions)
	{
		host.WriteCompleted(streamName, success, writeContext, assertPermissions);
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
}
