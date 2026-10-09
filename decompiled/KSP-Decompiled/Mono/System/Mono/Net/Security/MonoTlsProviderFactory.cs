using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Security.Cryptography.X509Certificates;
using Mono.Security.Interface;
using Mono.Unity;

namespace Mono.Net.Security;

internal static class MonoTlsProviderFactory
{
	private static object locker = new object();

	private static bool initialized;

	private static MonoTlsProvider defaultProvider;

	private static Dictionary<string, Tuple<Guid, string>> providerRegistration;

	private static Dictionary<Guid, MonoTlsProvider> providerCache;

	private static bool enableDebug;

	internal static readonly Guid UnityTlsId = new Guid("06414A97-74F6-488F-877B-A6CA9BBEB82E");

	internal static readonly Guid AppleTlsId = new Guid("981af8af-a3a3-419a-9f01-a518e3a17c1c");

	internal static readonly Guid BtlsId = new Guid("432d18c9-9348-4b90-bfbf-9f2a10e1f15b");

	internal static readonly Guid LegacyId = new Guid("809e77d5-56cc-4da8-b9f0-45e65ba9cceb");

	internal static bool IsInitialized
	{
		get
		{
			lock (locker)
			{
				return initialized;
			}
		}
	}

	internal static MonoTlsProvider GetProviderInternal()
	{
		lock (locker)
		{
			InitializeInternal();
			return defaultProvider;
		}
	}

	internal static void InitializeInternal()
	{
		lock (locker)
		{
			if (!initialized)
			{
				InitializeProviderRegistration();
				MonoTlsProvider monoTlsProvider;
				try
				{
					monoTlsProvider = CreateDefaultProviderImpl();
				}
				catch (Exception innerException)
				{
					throw new NotSupportedException("TLS Support not available.", innerException);
				}
				if (monoTlsProvider == null)
				{
					throw new NotSupportedException("TLS Support not available.");
				}
				if (!providerCache.ContainsKey(monoTlsProvider.ID))
				{
					providerCache.Add(monoTlsProvider.ID, monoTlsProvider);
				}
				X509Helper2.Initialize();
				defaultProvider = monoTlsProvider;
				initialized = true;
			}
		}
	}

	internal static void InitializeInternal(string provider)
	{
		lock (locker)
		{
			if (initialized)
			{
				throw new NotSupportedException("TLS Subsystem already initialized.");
			}
			defaultProvider = LookupProvider(provider, throwOnError: true);
			X509Helper2.Initialize();
			initialized = true;
		}
	}

	private static Type LookupProviderType(string name, bool throwOnError)
	{
		lock (locker)
		{
			InitializeProviderRegistration();
			if (!providerRegistration.TryGetValue(name, out var value))
			{
				if (throwOnError)
				{
					throw new NotSupportedException($"No such TLS Provider: `{name}'.");
				}
				return null;
			}
			Type type = Type.GetType(value.Item2, throwOnError: false);
			if (type == null && throwOnError)
			{
				throw new NotSupportedException($"Could not find TLS Provider: `{value.Item2}'.");
			}
			return type;
		}
	}

	private static MonoTlsProvider LookupProvider(string name, bool throwOnError)
	{
		lock (locker)
		{
			InitializeProviderRegistration();
			if (!providerRegistration.TryGetValue(name, out var value))
			{
				if (throwOnError)
				{
					throw new NotSupportedException($"No such TLS Provider: `{name}'.");
				}
				return null;
			}
			if (providerCache.TryGetValue(value.Item1, out var value2))
			{
				return value2;
			}
			Type type = Type.GetType(value.Item2, throwOnError: false);
			if (type == null && throwOnError)
			{
				throw new NotSupportedException($"Could not find TLS Provider: `{value.Item2}'.");
			}
			try
			{
				value2 = (MonoTlsProvider)Activator.CreateInstance(type, nonPublic: true);
			}
			catch (Exception innerException)
			{
				throw new NotSupportedException($"Unable to instantiate TLS Provider `{type}'.", innerException);
			}
			if (value2 == null)
			{
				if (throwOnError)
				{
					throw new NotSupportedException($"No such TLS Provider: `{name}'.");
				}
				return null;
			}
			providerCache.Add(value.Item1, value2);
			return value2;
		}
	}

	[Conditional("MONO_TLS_DEBUG")]
	private static void InitializeDebug()
	{
		if (Environment.GetEnvironmentVariable("MONO_TLS_DEBUG") != null)
		{
			enableDebug = true;
		}
	}

	[Conditional("MONO_TLS_DEBUG")]
	internal static void Debug(string message, params object[] args)
	{
		if (enableDebug)
		{
			Console.Error.WriteLine(message, args);
		}
	}

	private static void InitializeProviderRegistration()
	{
		lock (locker)
		{
			if (providerRegistration != null)
			{
				return;
			}
			providerRegistration = new Dictionary<string, Tuple<Guid, string>>();
			providerCache = new Dictionary<Guid, MonoTlsProvider>();
			if (UnityTls.IsSupported)
			{
				Tuple<Guid, string> value = new Tuple<Guid, string>(UnityTlsId, "Mono.Unity.UnityTlsProvider");
				providerRegistration.Add("default", value);
				providerRegistration.Add("unitytls", value);
				return;
			}
			Tuple<Guid, string> value2 = new Tuple<Guid, string>(AppleTlsId, "Mono.AppleTls.AppleTlsProvider");
			Tuple<Guid, string> value3 = new Tuple<Guid, string>(LegacyId, "Mono.Net.Security.LegacyTlsProvider");
			providerRegistration.Add("legacy", value3);
			Tuple<Guid, string> tuple = null;
			if (Platform.IsMacOS)
			{
				providerRegistration.Add("default", value2);
			}
			else if (tuple != null)
			{
				providerRegistration.Add("default", tuple);
			}
			else
			{
				providerRegistration.Add("default", value3);
			}
			providerRegistration.Add("apple", value2);
		}
	}

	private static MonoTlsProvider CreateDefaultProviderImpl()
	{
		string text = Environment.GetEnvironmentVariable("MONO_TLS_PROVIDER");
		if (string.IsNullOrEmpty(text))
		{
			text = "default";
		}
		return LookupProvider(text, throwOnError: true);
	}

	internal static MonoTlsProvider GetProvider()
	{
		return GetProviderInternal() ?? throw new NotSupportedException("No TLS Provider available.");
	}

	internal static bool IsProviderSupported(string name)
	{
		lock (locker)
		{
			InitializeProviderRegistration();
			return providerRegistration.ContainsKey(name);
		}
	}

	internal static MonoTlsProvider GetProvider(string name)
	{
		return LookupProvider(name, throwOnError: false);
	}

	internal static void Initialize()
	{
		InitializeInternal();
	}

	internal static void Initialize(string provider)
	{
		InitializeInternal(provider);
	}
}
