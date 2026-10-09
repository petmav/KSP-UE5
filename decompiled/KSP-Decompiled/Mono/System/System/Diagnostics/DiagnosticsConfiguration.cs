using System.Configuration;

namespace System.Diagnostics;

internal static class DiagnosticsConfiguration
{
	private static volatile SystemDiagnosticsSection configSection;

	private static volatile InitState initState;

	internal static SwitchElementsCollection SwitchSettings
	{
		get
		{
			Initialize();
			return configSection?.Switches;
		}
	}

	internal static bool AssertUIEnabled
	{
		get
		{
			Initialize();
			SystemDiagnosticsSection systemDiagnosticsSection = configSection;
			if (systemDiagnosticsSection != null && systemDiagnosticsSection.Assert != null)
			{
				return systemDiagnosticsSection.Assert.AssertUIEnabled;
			}
			return true;
		}
	}

	internal static string ConfigFilePath
	{
		get
		{
			Initialize();
			SystemDiagnosticsSection systemDiagnosticsSection = configSection;
			if (systemDiagnosticsSection != null)
			{
				return systemDiagnosticsSection.ElementInformation.Source;
			}
			return string.Empty;
		}
	}

	internal static string LogFileName
	{
		get
		{
			Initialize();
			SystemDiagnosticsSection systemDiagnosticsSection = configSection;
			if (systemDiagnosticsSection != null && systemDiagnosticsSection.Assert != null)
			{
				return systemDiagnosticsSection.Assert.LogFileName;
			}
			return string.Empty;
		}
	}

	internal static bool AutoFlush
	{
		get
		{
			Initialize();
			SystemDiagnosticsSection systemDiagnosticsSection = configSection;
			if (systemDiagnosticsSection != null && systemDiagnosticsSection.Trace != null)
			{
				return systemDiagnosticsSection.Trace.AutoFlush;
			}
			return false;
		}
	}

	internal static bool UseGlobalLock
	{
		get
		{
			Initialize();
			SystemDiagnosticsSection systemDiagnosticsSection = configSection;
			if (systemDiagnosticsSection != null && systemDiagnosticsSection.Trace != null)
			{
				return systemDiagnosticsSection.Trace.UseGlobalLock;
			}
			return true;
		}
	}

	internal static int IndentSize
	{
		get
		{
			Initialize();
			SystemDiagnosticsSection systemDiagnosticsSection = configSection;
			if (systemDiagnosticsSection != null && systemDiagnosticsSection.Trace != null)
			{
				return systemDiagnosticsSection.Trace.IndentSize;
			}
			return 4;
		}
	}

	internal static ListenerElementsCollection SharedListeners
	{
		get
		{
			Initialize();
			return configSection?.SharedListeners;
		}
	}

	internal static SourceElementsCollection Sources
	{
		get
		{
			Initialize();
			SystemDiagnosticsSection systemDiagnosticsSection = configSection;
			if (systemDiagnosticsSection != null && systemDiagnosticsSection.Sources != null)
			{
				return systemDiagnosticsSection.Sources;
			}
			return null;
		}
	}

	internal static SystemDiagnosticsSection SystemDiagnosticsSection
	{
		get
		{
			Initialize();
			return configSection;
		}
	}

	private static SystemDiagnosticsSection GetConfigSection()
	{
		object section = System.Configuration.PrivilegedConfigurationManager.GetSection("system.diagnostics");
		if (section is SystemDiagnosticsSection)
		{
			return (SystemDiagnosticsSection)section;
		}
		return null;
	}

	internal static bool IsInitializing()
	{
		return initState == InitState.Initializing;
	}

	internal static bool IsInitialized()
	{
		return initState == InitState.Initialized;
	}

	internal static bool CanInitialize()
	{
		if (initState != InitState.Initializing)
		{
			return !ConfigurationManagerInternalFactory.Instance.SetConfigurationSystemInProgress;
		}
		return false;
	}

	internal static void Initialize()
	{
		lock (TraceInternal.critSec)
		{
			if (initState != 0 || ConfigurationManagerInternalFactory.Instance.SetConfigurationSystemInProgress)
			{
				return;
			}
			initState = InitState.Initializing;
			try
			{
				configSection = GetConfigSection();
			}
			finally
			{
				initState = InitState.Initialized;
			}
		}
	}

	internal static void Refresh()
	{
		ConfigurationManager.RefreshSection("system.diagnostics");
		SystemDiagnosticsSection systemDiagnosticsSection = configSection;
		if (systemDiagnosticsSection != null)
		{
			if (systemDiagnosticsSection.Switches != null)
			{
				foreach (SwitchElement @switch in systemDiagnosticsSection.Switches)
				{
					@switch.ResetProperties();
				}
			}
			if (systemDiagnosticsSection.SharedListeners != null)
			{
				foreach (ListenerElement sharedListener in systemDiagnosticsSection.SharedListeners)
				{
					sharedListener.ResetProperties();
				}
			}
			if (systemDiagnosticsSection.Sources != null)
			{
				foreach (SourceElement source in systemDiagnosticsSection.Sources)
				{
					source.ResetProperties();
				}
			}
		}
		configSection = null;
		initState = InitState.NotInitialized;
		Initialize();
	}
}
